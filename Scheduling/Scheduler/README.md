# Scheduler

A reusable scheduling manager for the ESP32 IoT Framework.

Scheduler executes one-shot and repeating timers without blocking the main loop.

When a scheduled task becomes due, Scheduler publishes an event through EventBus.

It never performs application logic or controls hardware directly.

---

# Features

* One-shot timers
* Repeating timers
* Non-blocking scheduling
* Fixed-size task table
* EventBus integration
* Timer cancellation
* Task statistics
* Lightweight design
* Framework-friendly architecture

---

# Create the manager

```cpp
Scheduler scheduler;
```

---

# Initialize

```cpp
scheduler.begin(eventBus);
```

---

# Schedule a One-Shot Task

```cpp
scheduler.scheduleOnce(
    EventType::RelayStateChanged,
    5000
);
```

Publishes the event once after 5 seconds.

---

# Schedule a Repeating Task

```cpp
scheduler.scheduleRepeating(
    EventType::SyncStarted,
    10000
);
```

Publishes the event every 10 seconds.

---

# Cancel a Task

```cpp
scheduler.cancel(taskId);
```

Cancels a previously scheduled task.

---

# Cancel All Tasks

```cpp
scheduler.cancelAll();
```

Removes every scheduled task.

---

# Task Information

```cpp
uint32_t count =
    scheduler.getTaskCount();

bool active =
    scheduler.isScheduled(taskId);

bool busy =
    scheduler.isBusy();
```

---

# Main Loop

```cpp
void loop()
{
    scheduler.loop();
}
```

Scheduler checks pending timers during every loop iteration.

---

# Framework Lifecycle

```cpp
scheduler.begin(eventBus);

scheduler.loop();

scheduler.enable();

scheduler.disable();

scheduler.isEnabled();
```

---

# Event Flow

```text
Application
        │
        ▼
   Scheduler
        │
        ▼
    EventBus
        │
 ┌──────┼──────────┐
 ▼      ▼          ▼
Relay  Logger   SyncManager
```

Scheduler only publishes events.

Subscribers decide what action to perform.

---

# Notes

* Scheduler is completely non-blocking.
* Timer resolution is based on `millis()`.
* No `delay()` is used.
* One-shot timers execute once and are automatically removed.
* Repeating timers automatically reschedule themselves.
* Scheduler uses a fixed-size task table.
* Scheduler never allocates dynamic memory.
* Scheduler never controls hardware directly.
* Scheduler communicates only through EventBus.
* Additional scheduling modes (daily, weekly, cron, RTC, NTP) can be added in future without changing the public API.
