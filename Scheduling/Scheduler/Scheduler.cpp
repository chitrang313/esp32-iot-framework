#include "Scheduler.h"
#include <string.h>  // memcpy

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

Scheduler::Scheduler()
    : m_initialized(false)
    , m_enabled    (true)
    , m_eventBus   (nullptr)
    , m_nextTaskId (1u)
    , m_taskCount  (0u)
{
    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        m_tasks[i].active      = false;
        m_tasks[i].id          = 0u;
        m_tasks[i].payloadSize = 0u;
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Scheduler::begin(EventBus& eventBus)
{
    m_eventBus   = &eventBus;
    m_taskCount  = 0u;
    m_nextTaskId = 1u;

    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        m_tasks[i].active      = false;
        m_tasks[i].id          = 0u;
        m_tasks[i].payloadSize = 0u;
    }

    m_initialized = true;
    m_enabled     = true;
}

void Scheduler::loop()
{
    if (!m_initialized || !m_enabled) return;

    const uint32_t now = millis();

    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        ScheduledTask& task = m_tasks[i];

        if (!task.active)                    continue;
        if (!isTaskDue(now, task.nextFireMs)) continue;

        // Task is due — publish its event first, then update state.
        // Publishing before rescheduling ensures the event timestamp in the
        // EventBus entry reflects when the task actually fired.
        publishTaskEvent(task);

        if (task.repeating)
        {
            rescheduleTask(task, now);
        }
        else
        {
            // One-shot: free the slot immediately so it can be reused.
            task.active = false;
            task.id     = 0u;
            if (m_taskCount > 0u) --m_taskCount;
        }
    }
}

void Scheduler::enable()
{
    m_enabled = true;
}

void Scheduler::disable()
{
    m_enabled = false;
}

bool Scheduler::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Scheduling
// ---------------------------------------------------------------------------

SchedulerResult Scheduler::scheduleOnce(EventType   type,
                                         uint32_t    delayMs,
                                         uint32_t*   outTaskId,
                                         const void* payload,
                                         size_t      payloadSize)
{
    if (!m_initialized)           return SchedulerResult::NotInitialized;
    if (!m_enabled)               return SchedulerResult::Disabled;
    if (!isValidEventType(type))  return SchedulerResult::InvalidTask;

    const int8_t slot = findFreeSlot();
    if (slot < 0) return SchedulerResult::TaskTableFull;

    ScheduledTask& task = m_tasks[static_cast<uint8_t>(slot)];
    initTask(task, type, millis() + delayMs, delayMs, false, payload, payloadSize);

    if (outTaskId != nullptr) *outTaskId = task.id;
    ++m_taskCount;
    return SchedulerResult::Success;
}

SchedulerResult Scheduler::scheduleRepeating(EventType   type,
                                              uint32_t    intervalMs,
                                              uint32_t*   outTaskId,
                                              const void* payload,
                                              size_t      payloadSize)
{
    if (!m_initialized)           return SchedulerResult::NotInitialized;
    if (!m_enabled)               return SchedulerResult::Disabled;
    if (!isValidEventType(type))  return SchedulerResult::InvalidTask;
    // Zero interval would fire every loop() call, flooding EventBus.
    if (intervalMs == 0u)         return SchedulerResult::InvalidTask;

    const int8_t slot = findFreeSlot();
    if (slot < 0) return SchedulerResult::TaskTableFull;

    ScheduledTask& task = m_tasks[static_cast<uint8_t>(slot)];
    initTask(task, type, millis() + intervalMs, intervalMs, true, payload, payloadSize);

    if (outTaskId != nullptr) *outTaskId = task.id;
    ++m_taskCount;
    return SchedulerResult::Success;
}

SchedulerResult Scheduler::cancel(uint32_t taskId)
{
    if (!m_initialized)  return SchedulerResult::NotInitialized;
    if (taskId == 0u)    return SchedulerResult::InvalidTask;

    const int8_t slot = findTaskById(taskId);
    if (slot < 0) return SchedulerResult::TaskNotFound;

    m_tasks[static_cast<uint8_t>(slot)].active = false;
    m_tasks[static_cast<uint8_t>(slot)].id     = 0u;
    if (m_taskCount > 0u) --m_taskCount;
    return SchedulerResult::Success;
}

void Scheduler::cancelAll()
{
    if (!m_initialized) return;

    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        m_tasks[i].active = false;
        m_tasks[i].id     = 0u;
    }
    m_taskCount = 0u;
}

// ---------------------------------------------------------------------------
// Information
// ---------------------------------------------------------------------------

uint32_t Scheduler::getTaskCount() const { return m_taskCount; }
bool     Scheduler::isBusy()       const { return m_taskCount > 0u; }

bool Scheduler::isScheduled(uint32_t taskId) const
{
    if (taskId == 0u) return false;
    return findTaskById(taskId) >= 0;
}

// ---------------------------------------------------------------------------
// Private helpers — validation
// ---------------------------------------------------------------------------

bool Scheduler::isValidEventType(EventType type) const
{
    // EventType::None (uninitialised) and EventType::Count (sentinel) must
    // not be scheduled; they have no subscribers and would waste a task slot.
    return type != EventType::None && type < EventType::Count;
}

// ---------------------------------------------------------------------------
// Private helpers — task management
// ---------------------------------------------------------------------------

int8_t Scheduler::findFreeSlot() const
{
    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        if (!m_tasks[i].active) return static_cast<int8_t>(i);
    }
    return -1;
}

int8_t Scheduler::findTaskById(uint32_t id) const
{
    for (uint8_t i = 0u; i < MAX_TASKS; ++i)
    {
        if (m_tasks[i].active && m_tasks[i].id == id)
        {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

uint32_t Scheduler::allocateTaskId()
{
    const uint32_t id = m_nextTaskId;
    ++m_nextTaskId;
    // Skip 0: it is the sentinel for "no task / free slot".
    if (m_nextTaskId == 0u) m_nextTaskId = 1u;
    return id;
}

void Scheduler::initTask(ScheduledTask& task,
                          EventType   type,
                          uint32_t    firstFireMs,
                          uint32_t    intervalMs,
                          bool        repeating,
                          const void* payload,
                          size_t      payloadSize)
{
    task.id          = allocateTaskId();
    task.eventType   = type;
    task.intervalMs  = intervalMs;
    task.nextFireMs  = firstFireMs;  // millis() + delay/interval; natural overflow OK
    task.repeating   = repeating;
    task.active      = true;
    task._reserved   = 0u;

    if (payload != nullptr && payloadSize > 0u)
    {
        const size_t copySize = (payloadSize <= TASK_PAYLOAD_SIZE)
                                ? payloadSize
                                : TASK_PAYLOAD_SIZE;
        memcpy(task.payload, payload, copySize);
        task.payloadSize = copySize;
    }
    else
    {
        task.payloadSize = 0u;
    }
}

// ---------------------------------------------------------------------------
// Private helpers — timer processing
// ---------------------------------------------------------------------------

bool Scheduler::isTaskDue(uint32_t now, uint32_t nextFireMs) const
{
    // Unsigned subtraction wraps safely at 2^32, mirroring millis() rollover.
    // Casting to int32_t converts the wrap-around difference to a negative
    // value when nextFireMs is in the future — the task is not yet due.
    //
    // Example with rollover: now=5, nextFireMs=4294967294
    //   (uint32_t)(5 - 4294967294) = 7  →  (int32_t)(7) = 7 ≥ 0 → due ✓
    return static_cast<int32_t>(now - nextFireMs) >= 0;
}

void Scheduler::publishTaskEvent(const ScheduledTask& task)
{
    if (m_eventBus == nullptr) return;

    const void* payload = (task.payloadSize > 0u)
                          ? static_cast<const void*>(task.payload)
                          : nullptr;

    // EventSource::System: Scheduler is a framework-level module, not application code.
    m_eventBus->publish(task.eventType, EventSource::System, payload, task.payloadSize);
}

void Scheduler::rescheduleTask(ScheduledTask& task, uint32_t now)
{
    // Advance from the last scheduled time (not from now) to prevent drift:
    // if loop() ran 2ms late, the next interval is still measured from the
    // original fire time, not from the late actual time.
    task.nextFireMs += task.intervalMs;

    // If loop() ran so late that the task is STILL due after one advance,
    // skip the missed intervals via division rather than a while-loop.
    // This prevents a burst of EventBus publishes when recovering from lag.
    if (isTaskDue(now, task.nextFireMs))
    {
        // Calculate how many more intervals have already elapsed.
        const uint32_t remaining = now - task.nextFireMs;
        const uint32_t skip      = (remaining / task.intervalMs) + 1u;
        task.nextFireMs += skip * task.intervalMs;
        // Natural uint32_t overflow in the addition is intentional and correct:
        // isTaskDue() will return false for the wrapped future value.
    }
}
