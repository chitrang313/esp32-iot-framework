# EventBus - instruction-part5.md

# Part 5 - Framework Standards, Coding Standards & Future Roadmap

---

# Objective

This document defines the implementation standards for EventBus.

Every implementation must follow these rules to ensure the framework remains modular, scalable, reliable, and reusable.

---

# Framework Philosophy

EventBus is a framework module.

It is **not** an application.

It must never contain application-specific logic.

Bad Example

```cpp
if (relayChanged)
{
    logger.log(...);
    firebase.sync(...);
}
```

Good Example

```cpp
eventBus.publish(...);
```

Subscribers decide what to do.

EventBus only delivers events.

---

# Module Independence

EventBus must never directly communicate with

* RelayManager
* SwitchManager
* StatusLedManager
* IoTWiFiManager
* FirebaseManager
* PreferencesManager
* DeviceStateManager
* SyncManager
* LoggerManager
* Scheduler

Every manager communicates through EventBus.

EventBus never performs application logic.

---

# Responsibilities

EventBus owns

* Event Queue
* Subscriber Table
* Event Dispatch
* Event Validation

EventBus does NOT own

* Runtime State
* Hardware
* Cloud Communication
* Synchronization
* Logging
* Scheduling

---

# Public API Rules

The public API is considered stable.

Do NOT

* Rename public functions
* Change parameter order
* Remove overloads
* Remove enums
* Change return types

Maintain backward compatibility.

---

# Naming Convention

Classes

```text
PascalCase
```

Example

```text
EventBus
```

Functions

```text
camelCase
```

Examples

```text
begin()

publish()

subscribe()

unsubscribe()

loop()

enable()

disable()
```

Private members

```text
m_memberVariable
```

Private helpers

```text
camelCase()
```

Always use

```cpp
enum class
```

for enumerations.

---

# Documentation

Every public function should include

* Brief description
* Parameters
* Return value

Use Doxygen-style comments.

---

# Comment Style

Comments explain

WHY

not

WHAT

Good

```cpp
// Queue events to prevent recursive callback execution.
```

Bad

```cpp
// Publish event.
```

---

# Validation

Before publishing verify

* Manager initialized
* Manager enabled
* Valid event type
* Queue has capacity

Before subscribing verify

* Valid callback
* Valid event type

Before dispatch verify

* Valid subscriber
* Valid callback

Never crash.

---

# Error Handling

Every public function should return

```text
EventResult
```

Examples

```text
Success

QueueFull

AlreadySubscribed

NotSubscribed

InvalidEvent

InvalidSubscriber

Disabled

Busy

Failed
```

Never silently ignore failures.

Never throw exceptions.

---

# Memory Rules

Avoid

* malloc
* free
* new
* delete

Prefer

* Fixed-size event queue
* Fixed-size subscriber table
* constexpr
* Stack allocation

Suitable for long-running embedded systems.

---

# Performance Rules

Publishing must remain constant time.

Dispatch only a limited number of events during each

```cpp
loop();
```

Avoid

* Recursive dispatch
* Duplicate event delivery
* Blocking callbacks
* Busy waiting

Never use

```cpp
delay();
```

inside the library.

---

# Logging

EventBus must never call

```cpp
Serial.print();

Serial.println();
```

Future logging belongs to LoggerManager.

---

# Thread Model

Assume Arduino single-threaded execution.

Do not introduce

* FreeRTOS Tasks
* Mutexes
* Semaphores

unless explicitly requested.

---

# Internal Organization

Only one public class should exist.

Suggested internal sections

```text
Initialization

Subscription Management

Queue Management

Event Dispatch

Validation

Statistics

Diagnostics

Helpers
```

These are implementation details.

---

# Folder Structure

```text
Events/
└── EventBus/
    ├── EventBus.h
    ├── EventBus.cpp
    ├── README.md
    ├── instruction-part1.md
    ├── instruction-part2.md
    ├── instruction-part3.md
    ├── instruction-part4.md
    └── instruction-part5.md
```

Maintain this structure.

Do not create additional public files.

---

# Coding Standards

Use

* enum class
* const correctness
* helper functions
* descriptive names
* private members
* no duplicated code
* readable code

Prefer clarity over clever implementations.

---

# Future Features

The architecture should support

without changing the public API

* Event priorities
* Wildcard subscriptions
* Sticky events
* Delayed events
* Event persistence
* Event replay
* Event filtering
* Remote event forwarding
* Distributed event systems
* Diagnostics integration

Implementation is NOT required.

Only the architecture should support future expansion.

---

# Deliverables

Implement only

```text
EventBus.h

EventBus.cpp
```

The implementation must satisfy every requirement defined in

* instruction-part1.md
* instruction-part2.md
* instruction-part3.md
* instruction-part4.md
* instruction-part5.md

---

# Final Goal

EventBus becomes the communication backbone of the ESP32 IoT Framework.

Every framework module should publish events instead of directly calling other modules.

Subscribers receive only the events they are interested in.

Publishers remain completely independent of subscribers.

The framework becomes loosely coupled, modular, scalable, and easy to extend without changing existing modules.
