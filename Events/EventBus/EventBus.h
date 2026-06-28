#pragma once

// EventBus — the publish-subscribe communication backbone for the ESP32 IoT Framework.
//
// EventBus decouples framework modules.  Publishers post events without knowing
// who will receive them.  Subscribers react to events without knowing who
// published them.
//
// Key design decisions:
//   - Publishing is O(1): inserts into a fixed FIFO queue and returns immediately.
//     Subscriber callbacks are NEVER called inside publish().
//   - Dispatch happens only in loop(), processed FIFO, limited to a fixed count
//     per loop() call so execution time stays bounded.
//   - Payloads are copied inline (up to MAX_PAYLOAD_SIZE bytes) at publish time.
//     This keeps the dispatched Event fully valid in callbacks even when the
//     publisher's local variable is already out of scope.
//   - Subscriber table and event queue are fixed-size; no heap allocation.
//
// Module independence: EventBus includes NO other framework manager headers.
// It is completely generic and application-agnostic.
//
// Typical usage:
//   EventBus eventBus;
//   void setup() { eventBus.begin(); }
//   void loop()  { eventBus.loop();  }
//
//   // Publisher (e.g. RelayManager):
//   eventBus.publish(EventType::RelayStateChanged, EventSource::RelayManager,
//                    &relayEvent, sizeof(relayEvent));
//
//   // Subscriber (e.g. SyncManager):
//   eventBus.subscribe(EventType::RelayStateChanged, onRelayChanged, this);

#include <Arduino.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Capacity constants
//
// Defined at file scope (following FirebaseManager's convention) so application
// code can reference them without an EventBus instance.  Mirrored as static
// constexpr inside the class for API-discoverable access.
// ---------------------------------------------------------------------------

/// Maximum bytes copied from a payload argument into the Event struct.
/// Payloads larger than this are silently truncated; payloadSize reflects the
/// truncated length so subscribers know what they have.
static constexpr size_t EVENTBUS_MAX_PAYLOAD_SIZE = 64u;

/// Maximum events the queue can hold simultaneously.
static constexpr uint8_t EVENTBUS_QUEUE_CAPACITY  = 32u;

/// Maximum total registered subscriber callbacks across all event types.
static constexpr uint8_t EVENTBUS_MAX_SUBSCRIBERS = 32u;

// ---------------------------------------------------------------------------
// EventType
//
// Identifiers for every event the framework can publish.
// EventType::None and EventType::Count are reserved (not publishable).
// ---------------------------------------------------------------------------
enum class EventType : uint8_t
{
    None = 0,                   ///< Uninitialised / no event.

    // -- Hardware --
    RelayStateChanged,          ///< Relay output turned on or off.
    SwitchPressed,              ///< Physical switch input activated.
    SwitchReleased,             ///< Physical switch input deactivated.

    // -- Network --
    WiFiConnected,              ///< Station associated with access point.
    WiFiDisconnected,           ///< Station lost association.
    WiFiReconnecting,           ///< Station attempting to re-associate.

    // -- Cloud --
    FirebaseReady,              ///< Firebase authenticated and ready for reads/writes.
    FirebaseDisconnected,       ///< Firebase session lost.
    FirebaseError,              ///< Firebase entered an unrecoverable error state.

    // -- Synchronization --
    SyncStarted,                ///< SyncManager began processing the queue.
    SyncCompleted,              ///< All queued items synced successfully.
    SyncFailed,                 ///< One or more items could not be synced.

    // -- System --
    BootCompleted,              ///< Device finished initialisation.
    RestartRequested,           ///< A module or the application requested a restart.
    ConfigurationChanged,       ///< Persistent configuration was updated.

    // Sentinel — keep last; used for range validation only.
    Count
};

// ---------------------------------------------------------------------------
// EventSource
//
// Identifies which module published the event.  Subscribers may filter on
// source as well as type if needed.
// ---------------------------------------------------------------------------
enum class EventSource : uint8_t
{
    Unknown = 0,
    RelayManager,
    SwitchManager,
    IoTWiFiManager,
    FirebaseManager,
    SyncManager,
    DeviceStateManager,
    PreferencesManager,
    Application,
    System
};

// ---------------------------------------------------------------------------
// EventPriority
//
// Carried in every Event for future priority-based dispatch.  Phase 1
// processes events FIFO regardless of priority.
// ---------------------------------------------------------------------------
enum class EventPriority : uint8_t
{
    High   = 0,
    Normal = 1,
    Low    = 2
};

// ---------------------------------------------------------------------------
// EventResult
//
// Returned by every public operation.  Never silently fail.
// ---------------------------------------------------------------------------
enum class EventResult : uint8_t
{
    Success,            ///< Operation completed successfully.
    QueueFull,          ///< Event queue at capacity; event was dropped.
    AlreadySubscribed,  ///< (type, callback) pair already registered.
    NotSubscribed,      ///< (type, callback) pair not found for removal.
    InvalidEvent,       ///< EventType is None, Count, or out of range.
    InvalidSubscriber,  ///< Callback pointer is null.
    Disabled,           ///< Manager is disabled; operation rejected.
    NotInitialized,     ///< begin() has not been called.
    Busy,               ///< Operation deferred; try again next loop() tick.
    Failed              ///< Subscriber table full or other unrecoverable error.
};

// ---------------------------------------------------------------------------
// EventState
//
// Observable state of the EventBus dispatch engine.
// ---------------------------------------------------------------------------
enum class EventState : uint8_t
{
    Idle,         ///< No dispatch in progress.
    Dispatching,  ///< Inside loop(); executing subscriber callbacks.
    Disabled      ///< Manager disabled via disable().
};

// ---------------------------------------------------------------------------
// Event
//
// The unit of data transferred from publisher to subscriber.
//
// payload[] is an INLINE copy of the publisher's data — not a pointer.
// This means the Event is always self-contained: callbacks receive valid data
// even when the publisher's source variable has already gone out of scope.
//
// If the original payload exceeds MAX_PAYLOAD_SIZE bytes it is truncated;
// payloadSize reflects the actual bytes copied, not the original size.
//
// Subscribers should check payloadSize before reading payload[].
// To extract a typed struct: memcpy(&myStruct, event.payload, sizeof(myStruct))
// ---------------------------------------------------------------------------
struct Event
{
    EventType     type;          ///< What happened.
    EventSource   source;        ///< Who published it.
    EventPriority priority;      ///< Scheduling hint (future use; FIFO in Phase 1).
    uint8_t       _reserved;     ///< Explicit padding; available for future per-event flags.
    uint32_t      timestamp;     ///< millis() captured the instant publish() was called.
    size_t        payloadSize;   ///< Bytes valid in payload[]; 0 means no payload.
    uint8_t       payload[EVENTBUS_MAX_PAYLOAD_SIZE]; ///< Inline copy of publisher data.
};

// ---------------------------------------------------------------------------
// EventCallback
//
// Every subscriber must implement this signature.
//
//   event   — the dispatched Event (const-ref; no extra copy).
//   context — opaque pointer registered with subscribe(); use it to reach a
//             class instance from a plain-C function pointer.
// ---------------------------------------------------------------------------
using EventCallback = void (*)(const Event& event, void* context);

// ---------------------------------------------------------------------------
// EventBus
// ---------------------------------------------------------------------------

class EventBus
{
public:

    // Capacity mirrors for API-discoverable access.
    static constexpr size_t  MAX_PAYLOAD_SIZE = EVENTBUS_MAX_PAYLOAD_SIZE;
    static constexpr uint8_t QUEUE_CAPACITY   = EVENTBUS_QUEUE_CAPACITY;
    static constexpr uint8_t MAX_SUBSCRIBERS  = EVENTBUS_MAX_SUBSCRIBERS;

    // -----------------------------------------------------------------------
    // Construction
    //
    // Zero-initialises members.  No I/O, no network, no side effects.
    // -----------------------------------------------------------------------
    EventBus();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises the EventBus and resets all internal state.
     *
     * Must be called once from setup() before any publish or subscribe call.
     * Safe to call again to perform a full reset (clears queue, subscribers,
     * and all statistics).
     */
    void begin();

    /**
     * @brief Drives event dispatch — call from every Arduino loop() iteration.
     *
     * Dequeues and dispatches up to EVENTS_PER_LOOP events per call so that
     * loop() always returns in bounded time.  If more events are pending they
     * are processed on subsequent loop() calls.
     *
     * Callbacks are executed here, never inside publish().
     */
    void loop();

    /**
     * @brief Re-enables the manager after a disable() call.
     *
     * Resumes loop() dispatch and publish() acceptance from the current state.
     */
    void enable();

    /**
     * @brief Suspends publish() and loop() dispatch.
     *
     * Events published while disabled are rejected.  The queue and subscriber
     * table are preserved so operation resumes cleanly on enable().
     */
    void disable();

    /**
     * @brief Returns whether the manager currently accepts publish() calls.
     * @return true when enabled, false when disabled.
     */
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Publishing
    // -----------------------------------------------------------------------

    /**
     * @brief Queues an event for asynchronous delivery to all matching subscribers.
     *
     * Returns immediately after inserting into the queue.  Subscriber callbacks
     * are NEVER called inside publish() to prevent recursive dispatch and deep
     * call-stack growth.
     *
     * Up to MAX_PAYLOAD_SIZE bytes are copied from payload.  The original
     * pointer is not retained; the caller may free or modify it immediately
     * after publish() returns.
     *
     * @param type        Event type (must not be None or Count).
     * @param source      Module that is publishing the event.
     * @param payload     Optional pointer to event-specific data.  May be null.
     * @param payloadSize Number of bytes at payload.  0 is valid (no payload).
     * @return Success when queued, QueueFull when queue is at capacity,
     *         InvalidEvent for reserved types, Disabled/NotInitialized.
     */
    EventResult publish(EventType   type,
                        EventSource source,
                        const void* payload     = nullptr,
                        size_t      payloadSize = 0u);

    // -----------------------------------------------------------------------
    // Subscription management
    // -----------------------------------------------------------------------

    /**
     * @brief Registers a callback to receive events of the given type.
     *
     * Multiple callbacks may subscribe to the same event type; each receives
     * an independent delivery.  Duplicate (type, callback) pairs are rejected
     * to prevent double-delivery from accidental re-registration.
     *
     * @param type     EventType to listen for (must not be None or Count).
     * @param callback Function pointer to invoke on dispatch.  Must not be null.
     * @param context  Optional opaque pointer forwarded to callback; useful for
     *                 reaching a class instance from a plain-C function pointer.
     * @return Success on registration, AlreadySubscribed if duplicate,
     *         InvalidEvent for reserved types, InvalidSubscriber for null
     *         callback, Failed when the subscriber table is full.
     */
    EventResult subscribe(EventType     type,
                          EventCallback callback,
                          void*         context = nullptr);

    /**
     * @brief Removes a previously registered (type, callback) subscription.
     *
     * Safe to call during dispatch (from within a callback) — the entry is
     * cleared immediately and the ongoing dispatch loop skips it cleanly.
     *
     * @param type     EventType the subscription was made for.
     * @param callback Function pointer that was registered.
     * @return Success when removed, NotSubscribed when not found.
     */
    EventResult unsubscribe(EventType type, EventCallback callback);

    /**
     * @brief Removes all registered subscriptions across all event types.
     *
     * Safe to call during dispatch.  Remaining dispatch iterations for the
     * current event silently skip any cleared entries.
     */
    void unsubscribeAll();

    // -----------------------------------------------------------------------
    // State queries
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the current dispatch engine state.
     * @return Current EventState (Idle, Dispatching, or Disabled).
     */
    EventState getState() const;

    /**
     * @brief Returns true when the event queue is non-empty.
     *
     * Indicates that pending events are waiting for the next loop() call.
     *
     * @return true when at least one event is queued.
     */
    bool isBusy() const;

    /**
     * @brief Returns true when the event queue is empty.
     * @return true when no events are pending.
     */
    bool isIdle() const;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the total number of active subscriptions across all event types.
     * @return Current subscriber count.
     */
    uint32_t getSubscriberCount() const;

    /**
     * @brief Returns the current number of events waiting in the queue.
     * @return Current queue fill level.
     */
    uint32_t getQueueSize() const;

    /**
     * @brief Returns the total number of events successfully enqueued since begin().
     * @return Cumulative published count.
     */
    uint32_t getPublishedCount() const;

    /**
     * @brief Returns the total number of events dispatched to subscribers since begin().
     * @return Cumulative dispatched count.
     */
    uint32_t getDispatchedCount() const;

    /**
     * @brief Returns the total number of events dropped due to a full queue since begin().
     * @return Cumulative dropped count.
     */
    uint32_t getDroppedCount() const;

private:

    // -----------------------------------------------------------------------
    // SubscriberEntry
    //
    // One slot in the flat subscriber table.  A flat table (vs. per-EventType
    // arrays) saves memory when most event types have few or no subscribers.
    // Dispatch scans O(MAX_SUBSCRIBERS) which is O(32) — constant in practice.
    // -----------------------------------------------------------------------
    struct SubscriberEntry
    {
        EventCallback callback;   ///< Function to call on dispatch; null = unused.
        void*         context;    ///< Opaque pointer forwarded to callback.
        EventType     eventType;  ///< Filter; only events of this type are delivered.
        bool          inUse;      ///< true = slot occupied.
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------
    bool       m_initialized;
    bool       m_enabled;
    bool       m_dispatching;  ///< true while loop() is executing callbacks.
    EventState m_state;

    // -----------------------------------------------------------------------
    // FIFO event queue (circular buffer)
    // -----------------------------------------------------------------------
    Event   m_queue[QUEUE_CAPACITY];
    uint8_t m_queueHead;   ///< Index of the oldest (next to dispatch) event.
    uint8_t m_queueTail;   ///< Index of the next empty slot.
    uint8_t m_queueCount;  ///< Number of events currently in queue.

    // -----------------------------------------------------------------------
    // Subscriber table
    // -----------------------------------------------------------------------
    SubscriberEntry m_subscribers[MAX_SUBSCRIBERS];
    uint8_t         m_subscriberCount;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    uint32_t m_publishedCount;
    uint32_t m_dispatchedCount;
    uint32_t m_droppedCount;

    // -----------------------------------------------------------------------
    // Private helpers — queue
    // -----------------------------------------------------------------------

    /// Insert an event at the tail of the FIFO queue.
    /// Returns false (without modifying state) if the queue is full.
    bool enqueueEvent(const Event& event);

    /// Remove the oldest event from the queue and copy it into outEvent.
    /// Returns false if the queue is empty.
    bool dequeueEvent(Event& outEvent);

    // -----------------------------------------------------------------------
    // Private helpers — dispatch
    // -----------------------------------------------------------------------

    /// Deliver event to every matching subscriber, in table order.
    /// Continues to subsequent subscribers when one has been removed during
    /// dispatch (inUse == false) or has a null callback.
    void dispatchEvent(const Event& event);

    // -----------------------------------------------------------------------
    // Private helpers — validation
    // -----------------------------------------------------------------------

    /// Returns Success when type is a valid, publishable EventType.
    /// Rejects None and Count — both are reserved sentinels.
    EventResult validateEventType(EventType type) const;
};
