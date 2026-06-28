#pragma once

// Scheduler — the timing engine for the ESP32 IoT Framework.
//
// Manages one-shot and repeating timers.  When a timer expires, Scheduler
// publishes the configured event through EventBus.  It never controls hardware
// or calls other framework managers directly.
//
// Architecture:
//   Application → Scheduler → EventBus → Framework modules
//
// Subscribers decide what action to take; Scheduler only drives the clock.
//
// millis() rollover is handled correctly: due-time comparison uses signed
// arithmetic on unsigned subtraction, which wraps safely at 2^32.
//
// Typical usage:
//   Scheduler scheduler;
//   void setup() { eventBus.begin(); scheduler.begin(eventBus); }
//   void loop()  { eventBus.loop(); scheduler.loop(); }
//
//   uint32_t taskId;
//   scheduler.scheduleRepeating(EventType::SyncStarted, 10000, &taskId);
//   scheduler.scheduleOnce(EventType::BootCompleted, 500);

#include <Arduino.h>
#include <stdint.h>
#include "../../Events/EventBus/EventBus.h"

// ---------------------------------------------------------------------------
// Capacity constants — file-scope so application code can reference them
// without an instance, following the framework convention.
// ---------------------------------------------------------------------------

/// Maximum number of simultaneously scheduled tasks.
static constexpr uint8_t SCHEDULER_MAX_TASKS = 16u;

/// Maximum payload bytes stored inline per task and forwarded to EventBus.
static constexpr size_t SCHEDULER_TASK_PAYLOAD_SIZE = 64u;

// ---------------------------------------------------------------------------
// SchedulerResult
//
// Returned by all public operations.  Never silently fail.
// ---------------------------------------------------------------------------
enum class SchedulerResult : uint8_t
{
    Success,          ///< Operation completed.
    InvalidTask,      ///< EventType is None/Count, intervalMs is 0, or taskId is 0.
    TaskNotFound,     ///< cancel() could not locate the given task ID.
    TaskTableFull,    ///< All MAX_TASKS slots are occupied.
    Disabled,         ///< Manager is disabled; scheduling rejected.
    NotInitialized,   ///< begin() has not been called.
    Failed            ///< Unrecoverable internal error.
};

// ---------------------------------------------------------------------------
// Scheduler
// ---------------------------------------------------------------------------

class Scheduler
{
public:

    // Capacity mirrors for API-discoverable access.
    static constexpr uint8_t MAX_TASKS         = SCHEDULER_MAX_TASKS;
    static constexpr size_t  TASK_PAYLOAD_SIZE = SCHEDULER_TASK_PAYLOAD_SIZE;

    // -----------------------------------------------------------------------
    // Construction
    //
    // Zero-initialises all members.  No I/O, no EventBus access.
    // -----------------------------------------------------------------------
    Scheduler();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises the Scheduler and associates it with an EventBus instance.
     *
     * Must be called once from setup() after eventBus.begin().
     * Clears the task table and resets all statistics.
     * Safe to call again for a full reset.
     *
     * @param eventBus Reference to the application's EventBus.
     */
    void begin(EventBus& eventBus);

    /**
     * @brief Checks all active tasks and fires any that are due.
     *
     * Must be called from every Arduino loop() iteration.
     * Execution time is bounded: only active tasks are inspected.
     * Never blocks; never uses delay().
     */
    void loop();

    /**
     * @brief Re-enables scheduling after a disable() call.
     *
     * Pending tasks are preserved; they resume firing on the next loop() tick.
     */
    void enable();

    /**
     * @brief Pauses all timer processing without clearing the task table.
     *
     * No events are published while disabled.  Existing tasks remain
     * scheduled and resume when enable() is called.
     */
    void disable();

    /**
     * @brief Returns whether the Scheduler is currently active.
     * @return true when enabled, false when disabled.
     */
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Scheduling
    // -----------------------------------------------------------------------

    /**
     * @brief Schedules a one-shot event to be published after a delay.
     *
     * The event is published once when the delay elapses, then the task is
     * automatically removed.
     *
     * @param type        EventType to publish.  Must not be None or Count.
     * @param delayMs     Milliseconds to wait before publishing.  0 fires on
     *                    the very next loop() call.
     * @param outTaskId   Optional: receives the assigned task ID so the task
     *                    can be cancelled later.  Pass nullptr to ignore.
     * @param payload     Optional data copied into the task and forwarded with
     *                    the event.  Must outlive this call (copied immediately).
     * @param payloadSize Bytes at payload; truncated to TASK_PAYLOAD_SIZE.
     * @return Success, InvalidTask, TaskTableFull, Disabled, or NotInitialized.
     */
    SchedulerResult scheduleOnce(EventType   type,
                                  uint32_t    delayMs,
                                  uint32_t*   outTaskId   = nullptr,
                                  const void* payload     = nullptr,
                                  size_t      payloadSize = 0u);

    /**
     * @brief Schedules an event to be published repeatedly at a fixed interval.
     *
     * The first publication happens after the first interval elapses.
     * Subsequent fires are calculated from the previous scheduled time (not
     * from when loop() actually runs) to prevent drift accumulation.
     *
     * If loop() runs late and multiple intervals are missed, only the most
     * recent expiry fires; excess missed fires are skipped to prevent burst
     * flooding of EventBus.
     *
     * @param type        EventType to publish.  Must not be None or Count.
     * @param intervalMs  Milliseconds between publications.  Must be > 0.
     * @param outTaskId   Optional: receives the assigned task ID.
     * @param payload     Optional payload; copied immediately.
     * @param payloadSize Bytes at payload; truncated to TASK_PAYLOAD_SIZE.
     * @return Success, InvalidTask, TaskTableFull, Disabled, or NotInitialized.
     */
    SchedulerResult scheduleRepeating(EventType   type,
                                       uint32_t    intervalMs,
                                       uint32_t*   outTaskId   = nullptr,
                                       const void* payload     = nullptr,
                                       size_t      payloadSize = 0u);

    /**
     * @brief Cancels a previously scheduled task by its task ID.
     *
     * Safe to call while the manager is disabled.
     * Safe to call from within an EventBus subscriber callback.
     *
     * @param taskId ID returned by scheduleOnce() or scheduleRepeating().
     *               0 is never a valid task ID.
     * @return Success when removed, TaskNotFound when ID is not active,
     *         InvalidTask for ID == 0, NotInitialized if begin() not called.
     */
    SchedulerResult cancel(uint32_t taskId);

    /**
     * @brief Cancels all scheduled tasks and resets the task counter.
     *
     * Safe to call while the manager is disabled.
     * Task IDs previously returned by schedule calls become invalid.
     */
    void cancelAll();

    // -----------------------------------------------------------------------
    // Information
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the number of currently active scheduled tasks.
     * @return Active task count in range [0, MAX_TASKS].
     */
    uint32_t getTaskCount() const;

    /**
     * @brief Returns true if a task with the given ID is currently active.
     * @param taskId ID to look up.
     * @return true when the task exists and has not yet fired (one-shot) or
     *         been cancelled.
     */
    bool isScheduled(uint32_t taskId) const;

    /**
     * @brief Returns true when at least one task is currently scheduled.
     * @return true when the task table is non-empty.
     */
    bool isBusy() const;

private:

    // -----------------------------------------------------------------------
    // ScheduledTask
    //
    // 4-byte fields first for natural alignment; 1-byte fields grouped to
    // minimise padding; explicit _reserved byte documents the padding slot.
    // -----------------------------------------------------------------------
    struct ScheduledTask
    {
        uint32_t  id;          ///< Unique ID allocated at schedule time; 0 = slot free.
        uint32_t  intervalMs;  ///< Delay (once) or period (repeating) in milliseconds.
        uint32_t  nextFireMs;  ///< millis() value at which to fire next.
        size_t    payloadSize; ///< Bytes valid in payload[]; 0 = no payload.
        EventType eventType;   ///< Event published when this task fires.
        bool      repeating;   ///< true = reschedule after firing; false = one-shot.
        bool      active;      ///< true = slot in use.
        uint8_t   _reserved;   ///< Explicit padding for future flags.
        uint8_t   payload[SCHEDULER_TASK_PAYLOAD_SIZE]; ///< Inline payload copy.
    };

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    bool      m_initialized;
    bool      m_enabled;
    EventBus* m_eventBus;
    uint32_t  m_nextTaskId;   ///< Auto-incrementing source for new task IDs; never 0.

    // -----------------------------------------------------------------------
    // Task table
    // -----------------------------------------------------------------------
    ScheduledTask m_tasks[MAX_TASKS];
    uint8_t       m_taskCount;  ///< Number of slots currently active.

    // -----------------------------------------------------------------------
    // Private helpers — validation
    // -----------------------------------------------------------------------

    /// Returns true when type is a concrete, publishable EventType.
    bool isValidEventType(EventType type) const;

    // -----------------------------------------------------------------------
    // Private helpers — task management
    // -----------------------------------------------------------------------

    /// Scans the task table for the first inactive slot.
    /// Returns the slot index, or -1 when the table is full.
    int8_t findFreeSlot() const;

    /// Scans the task table for an active task with the given ID.
    /// Returns the slot index, or -1 when not found.
    int8_t findTaskById(uint32_t id) const;

    /// Allocates and returns the next unique task ID, skipping 0 on wrap.
    uint32_t allocateTaskId();

    /// Populates a free task slot from the shared scheduling parameters.
    void initTask(ScheduledTask& task,
                  EventType   type,
                  uint32_t    firstFireMs,
                  uint32_t    intervalMs,
                  bool        repeating,
                  const void* payload,
                  size_t      payloadSize);

    // -----------------------------------------------------------------------
    // Private helpers — timer processing
    // -----------------------------------------------------------------------

    /// Returns true when task.nextFireMs has been reached by now.
    /// Uses signed comparison of unsigned subtraction to handle millis() rollover.
    bool isTaskDue(uint32_t now, uint32_t nextFireMs) const;

    /// Publishes the task's event through EventBus.
    void publishTaskEvent(const ScheduledTask& task);

    /// Advances nextFireMs for a repeating task to the next future expiry,
    /// skipping missed intervals via division to avoid burst catch-up.
    void rescheduleTask(ScheduledTask& task, uint32_t now);
};
