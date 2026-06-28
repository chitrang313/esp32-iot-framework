# Scheduler - instruction.md

# Scheduler

---

# Objective

Develop a reusable, production-quality Scheduler for the ESP32 IoT Framework.

Scheduler allows applications to execute actions at specific times or after specified delays.

Scheduler never controls hardware directly.

It publishes scheduled events through EventBus.

---

# Deliverables

Create only these files.

```text
Scheduling/
└── Scheduler/
    ├── Scheduler.h
    ├── Scheduler.cpp
    ├── README.md
    └── instruction.md
```

Do not create additional public headers.

---

# Purpose

Scheduler provides

* One-shot timers
* Repeating timers
* Delayed execution
* Periodic tasks
* Event scheduling

Scheduler becomes the timing engine of the framework.

---

# Responsibilities

Scheduler is responsible for

* Creating scheduled tasks
* Executing timers
* Publishing scheduled events
* Managing timer lifecycle
* Timer validation

Scheduler is NOT responsible for

* Relay control
* Runtime state
* WiFi
* Firebase
* Synchronization
* Logging
* Business logic

---

# Architecture

```text
Application

↓

Scheduler

↓

EventBus

↓

Framework Modules
```

Scheduler never calls other managers directly.

Everything happens through EventBus.

---

# Lifecycle

Follow framework standards.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Timer processing happens only inside

```cpp
loop();
```

Never block execution.

---

# Public API

Lifecycle

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Scheduling

```cpp
scheduleOnce();

scheduleRepeating();

cancel();

cancelAll();
```

Information

```cpp
getTaskCount();

isScheduled();

isBusy();
```

---

# Timer Types

Support

One Shot

```text
Wait 5 seconds

↓

Execute once
```

Repeating

```text
Every 10 seconds

↓

Execute repeatedly
```

---

# Timer Resolution

Use

```cpp
millis()
```

Do not use

```cpp
delay()
```

Scheduler must remain non-blocking.

---

# EventBus Integration

Scheduler publishes events.

Example

```text
08:00

↓

Scheduler

↓

EventBus.publish()

↓

RelayManager
```

Scheduler does not know who receives the event.

---

# Timer Storage

Use a fixed-size task table.

Example

```cpp
MAX_TASKS = 16
```

Avoid dynamic memory allocation.

---

# Task Information

Each scheduled task should contain

* Task ID
* Event Type
* Delay / Interval
* Next Execution Time
* Repeat Flag
* Active Flag
* Optional Payload

Architecture should support future expansion.

---

# Validation

Validate

* Manager initialized
* Manager enabled
* Valid task
* Available task slot

Return

```text
SchedulerResult
```

Never crash.

---

# Error Handling

Examples

```text
Success

InvalidTask

TaskNotFound

TaskTableFull

Disabled

Failed
```

Never throw exceptions.

---

# Memory

Avoid

* malloc
* free
* new
* delete

Use fixed-size arrays.

Suitable for long-running embedded systems.

---

# Performance

Never block

```cpp
loop();
```

Check only active tasks.

Execute only tasks that are due.

Avoid unnecessary processing.

---

# Event Publishing

When a timer expires

```text
Task Due

↓

EventBus.publish()

↓

Subscribers
```

Scheduler never performs application actions itself.

---

# Dependencies

Scheduler may communicate only with

* EventBus

Scheduler must never communicate directly with

* RelayManager
* SwitchManager
* StatusLedManager
* IoTWiFiManager
* FirebaseManager
* PreferencesManager
* DeviceStateManager
* SyncManager
* LoggerManager

---

# Documentation

Every public function should include

* Brief description
* Parameters
* Return value

Use Doxygen-style comments.

---

# Coding Standards

Use

* enum class
* const correctness
* helper functions
* descriptive names
* private members
* readable code

Prefer clarity over clever implementations.

---

# Future Compatibility

Architecture should support

* Daily schedules
* Weekly schedules
* Cron-style schedules
* RTC integration
* NTP synchronization
* Time zones
* Task priorities
* Persistent schedules
* Task callbacks

without changing the public API.

Implementation is NOT required.

---

# Final Goal

Scheduler becomes the timing engine of the ESP32 IoT Framework.

Applications create timers.

Scheduler monitors time using `millis()`.

When a timer expires, Scheduler publishes an event through EventBus.

Subscribers decide what action to take.

Scheduler remains lightweight, reusable, non-blocking, and independent of application logic.
