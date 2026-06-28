#include "EventBus.h"
#include <string.h>  // memcpy, memset

// Max events dispatched per single loop() call.  Prevents a burst of events
// from monopolising the Arduino main loop at the expense of other managers.
static constexpr uint8_t EVENTS_PER_LOOP = 5u;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

EventBus::EventBus()
    : m_initialized   (false)
    , m_enabled       (true)
    , m_dispatching   (false)
    , m_state         (EventState::Idle)
    , m_queueHead     (0u)
    , m_queueTail     (0u)
    , m_queueCount    (0u)
    , m_subscriberCount(0u)
    , m_publishedCount (0u)
    , m_dispatchedCount(0u)
    , m_droppedCount   (0u)
{
    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        m_subscribers[i].inUse    = false;
        m_subscribers[i].callback = nullptr;
        m_subscribers[i].context  = nullptr;
    }
    // Queue array is not explicitly zero-initialised here; m_queueCount == 0
    // guarantees no stale entry is ever read.
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void EventBus::begin()
{
    m_queueHead       = 0u;
    m_queueTail       = 0u;
    m_queueCount      = 0u;
    m_dispatching     = false;
    m_publishedCount  = 0u;
    m_dispatchedCount = 0u;
    m_droppedCount    = 0u;

    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        m_subscribers[i].inUse    = false;
        m_subscribers[i].callback = nullptr;
        m_subscribers[i].context  = nullptr;
    }
    m_subscriberCount = 0u;

    m_initialized = true;
    m_enabled     = true;
    m_state       = EventState::Idle;
}

void EventBus::loop()
{
    if (!m_initialized || !m_enabled) return;

    // Guard against recursive re-entry: if a subscriber callback calls loop()
    // (directly or via another manager that calls loop()), bail immediately.
    // Events queued during dispatch will be picked up on the next loop() tick.
    if (m_dispatching) return;

    m_dispatching = true;
    m_state       = EventState::Dispatching;

    uint8_t processed = 0u;
    while (m_queueCount > 0u && processed < EVENTS_PER_LOOP)
    {
        Event event;
        if (dequeueEvent(event))
        {
            dispatchEvent(event);
            ++m_dispatchedCount;
            ++processed;
        }
    }

    m_dispatching = false;
    m_state       = EventState::Idle;
}

void EventBus::enable()
{
    m_enabled = true;
    if (m_initialized)
    {
        m_state = EventState::Idle;
    }
}

void EventBus::disable()
{
    m_enabled = false;
    m_state   = EventState::Disabled;
}

bool EventBus::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Publishing
// ---------------------------------------------------------------------------

EventResult EventBus::publish(EventType   type,
                               EventSource source,
                               const void* payload,
                               size_t      payloadSize)
{
    if (!m_initialized) return EventResult::NotInitialized;
    if (!m_enabled)     return EventResult::Disabled;

    const EventResult typeCheck = validateEventType(type);
    if (typeCheck != EventResult::Success) return typeCheck;

    if (m_queueCount >= QUEUE_CAPACITY)
    {
        ++m_droppedCount;
        return EventResult::QueueFull;
    }

    // Zero-initialise the entire Event so unset fields are deterministic
    // and future extensions that read new fields get a safe value.
    Event event = {};
    event.type      = type;
    event.source    = source;
    event.priority  = EventPriority::Normal;
    event.timestamp = millis();

    if (payload != nullptr && payloadSize > 0u)
    {
        // Truncate silently rather than rejecting, so callers don't need to
        // check MAX_PAYLOAD_SIZE themselves for oversized but still useful data.
        const size_t copySize = (payloadSize <= MAX_PAYLOAD_SIZE)
                                ? payloadSize
                                : MAX_PAYLOAD_SIZE;
        memcpy(event.payload, payload, copySize);
        event.payloadSize = copySize;
    }
    // else: payloadSize stays 0 from zero-init; payload[] stays all-zero.

    if (!enqueueEvent(event))
    {
        // Double-checked: queue was non-full above, but enqueue failed anyway.
        ++m_droppedCount;
        return EventResult::QueueFull;
    }

    ++m_publishedCount;
    return EventResult::Success;
}

// ---------------------------------------------------------------------------
// Subscription management
// ---------------------------------------------------------------------------

EventResult EventBus::subscribe(EventType     type,
                                 EventCallback callback,
                                 void*         context)
{
    if (!m_initialized)        return EventResult::NotInitialized;
    if (callback == nullptr)   return EventResult::InvalidSubscriber;

    const EventResult typeCheck = validateEventType(type);
    if (typeCheck != EventResult::Success) return typeCheck;

    // Reject duplicate (type, callback) pairs.  Allowing duplicates would
    // deliver the same event twice to the same function on every dispatch.
    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        if (m_subscribers[i].inUse          &&
            m_subscribers[i].eventType == type &&
            m_subscribers[i].callback  == callback)
        {
            return EventResult::AlreadySubscribed;
        }
    }

    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        if (!m_subscribers[i].inUse)
        {
            m_subscribers[i].eventType = type;
            m_subscribers[i].callback  = callback;
            m_subscribers[i].context   = context;
            m_subscribers[i].inUse     = true;
            ++m_subscriberCount;
            return EventResult::Success;
        }
    }

    return EventResult::Failed;  // subscriber table is full
}

EventResult EventBus::unsubscribe(EventType type, EventCallback callback)
{
    if (!m_initialized)      return EventResult::NotInitialized;
    if (callback == nullptr) return EventResult::InvalidSubscriber;

    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        if (m_subscribers[i].inUse          &&
            m_subscribers[i].eventType == type &&
            m_subscribers[i].callback  == callback)
        {
            // Clear the slot before decrementing the count so that a
            // concurrent (callback-triggered) scan of the table never
            // reads a partially-cleared entry with inUse still true.
            m_subscribers[i].inUse    = false;
            m_subscribers[i].callback = nullptr;
            m_subscribers[i].context  = nullptr;
            if (m_subscriberCount > 0u)
            {
                --m_subscriberCount;
            }
            return EventResult::Success;
        }
    }

    return EventResult::NotSubscribed;
}

void EventBus::unsubscribeAll()
{
    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        m_subscribers[i].inUse    = false;
        m_subscribers[i].callback = nullptr;
        m_subscribers[i].context  = nullptr;
    }
    m_subscriberCount = 0u;
    // Safe to call during dispatch: the dispatchEvent() scan checks inUse
    // before invoking each callback, so cleared entries are simply skipped.
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

EventState EventBus::getState()  const { return m_state;       }
bool       EventBus::isBusy()    const { return m_queueCount > 0u; }
bool       EventBus::isIdle()    const { return m_queueCount == 0u; }

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

uint32_t EventBus::getSubscriberCount() const { return m_subscriberCount;  }
uint32_t EventBus::getQueueSize()       const { return m_queueCount;        }
uint32_t EventBus::getPublishedCount()  const { return m_publishedCount;    }
uint32_t EventBus::getDispatchedCount() const { return m_dispatchedCount;   }
uint32_t EventBus::getDroppedCount()    const { return m_droppedCount;      }

// ---------------------------------------------------------------------------
// Private — queue helpers
// ---------------------------------------------------------------------------

bool EventBus::enqueueEvent(const Event& event)
{
    if (m_queueCount >= QUEUE_CAPACITY) return false;

    m_queue[m_queueTail] = event;
    m_queueTail = static_cast<uint8_t>((m_queueTail + 1u) % QUEUE_CAPACITY);
    ++m_queueCount;
    return true;
}

bool EventBus::dequeueEvent(Event& outEvent)
{
    if (m_queueCount == 0u) return false;

    // Copy to caller's stack frame BEFORE advancing the head pointer so that
    // publish() cannot reclaim and overwrite this slot while the copy is in
    // progress (single-threaded Arduino — no real race, but keeps logic clear).
    outEvent    = m_queue[m_queueHead];
    m_queueHead = static_cast<uint8_t>((m_queueHead + 1u) % QUEUE_CAPACITY);
    --m_queueCount;
    return true;
}

// ---------------------------------------------------------------------------
// Private — dispatch
// ---------------------------------------------------------------------------

void EventBus::dispatchEvent(const Event& event)
{
    for (uint8_t i = 0u; i < MAX_SUBSCRIBERS; ++i)
    {
        // Re-check inUse on every iteration: a subscriber callback may have
        // called unsubscribe() or unsubscribeAll(), clearing later entries.
        if (!m_subscribers[i].inUse)                    continue;
        if (m_subscribers[i].eventType != event.type)   continue;
        if (m_subscribers[i].callback  == nullptr)      continue;

        // Invoke callback.  If a prior subscriber called unsubscribeAll(),
        // this entry is already skipped by the inUse guard above, so no
        // subscriber is called more than once per event.
        m_subscribers[i].callback(event, m_subscribers[i].context);

        // No exception or error capture here — C function-pointer callbacks
        // don't throw.  The loop always continues to the next subscriber,
        // ensuring one subscriber's behaviour never silences the rest.
    }
}

// ---------------------------------------------------------------------------
// Private — validation
// ---------------------------------------------------------------------------

EventResult EventBus::validateEventType(EventType type) const
{
    // EventType::None (0) is uninitialised; EventType::Count is a sentinel.
    // Neither should ever be published or subscribed to.
    if (type == EventType::None || type >= EventType::Count)
    {
        return EventResult::InvalidEvent;
    }
    return EventResult::Success;
}
