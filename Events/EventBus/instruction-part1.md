# EventBus - instruction-part1.md

# Part 1 - Foundation & Architecture

---

# Objective

Develop a reusable, production-quality EventBus for the ESP32 IoT Framework.

EventBus provides a lightweight publish-subscribe communication mechanism between framework modules.

Modules should communicate through EventBus instead of directly calling each other.

---

# Deliverables

Create only these files.

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

Do not create additional public headers.

---

# Purpose

EventBus allows independent modules to exchange information without creating direct dependencies.

Example

```text
RelayManager

↓

EventBus

↓

LoggerManager

↓

SyncManager

↓

Application
```

No module knows who is listening.

---

# Responsibilities

EventBus is responsible for

* Publishing events
* Registering subscribers
* Removing subscribers
* Dispatching events
* Supporting multiple listeners
* Delivering events safely
* Managing subscriptions

EventBus is NOT responsible for

* Hardware control
* Runtime state
* Persistent storage
* WiFi communication
* Firebase communication
* Synchronization
* Logging
* Scheduling

---

# Design Philosophy

Modules should never call each other directly.

Bad

```text
RelayManager

↓

LoggerManager

↓

SyncManager
```

Good

```text
RelayManager

↓

EventBus

↓

LoggerManager

↓

SyncManager
```

Loose coupling is the primary goal.

---

# Architecture

```text
RelayManager
        │
SwitchManager
        │
IoTWiFiManager
        │
FirebaseManager
        │
DeviceStateManager
        │
SyncManager
        │
        ▼
      EventBus
        │
 ┌──────┼───────────┐
 ▼      ▼           ▼
Logger  Scheduler  Application
```

Every module may publish events.

Every module may subscribe.

---

# Lifecycle

Follow the framework standard.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

loop() processes queued events.

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

Subscription

```cpp
subscribe();

unsubscribe();

unsubscribeAll();
```

Publishing

```cpp
publish();
```

Information

```cpp
getSubscriberCount();

getQueueSize();

isBusy();
```

---

# Event Model

Every event should contain

* Event Type
* Source
* Timestamp
* Optional Payload

Future versions may extend the payload.

---

# Enumerations

Use enum class wherever possible.

Examples

```text
EventType

EventSource

EventPriority

EventResult

EventState
```

Never use raw integers.

---

# Event Queue

Events should be queued.

Publishing should never immediately execute subscribers.

Example

```text
Publish Event

↓

Queue

↓

loop()

↓

Dispatch
```

This prevents deep call chains and recursion.

---

# Validation

Validate

* Manager initialized
* Manager enabled
* Valid subscriber
* Valid event
* Queue capacity

Return EventResult.

Never crash.

---

# Logging

Never use

* Serial.print()
* Serial.println()

Future logging belongs to LoggerManager.

---

# Memory

Avoid

* new
* delete
* malloc
* free

Use fixed-size arrays.

Suitable for long-running embedded systems.

---

# Performance

Publishing must be fast.

Dispatch should occur inside

```cpp
loop();
```

Never block execution.

---

# Future Compatibility

Architecture should support

* Event filtering
* Event priorities
* Delayed events
* Sticky events
* Remote events
* Broadcast events

without changing the public API.

---

# Final Goal

EventBus becomes the communication backbone of the framework.

Every module publishes events.

Every module subscribes to events.

No framework module should directly depend on another module unless ownership requires it.

The framework remains modular, loosely coupled, and easily extensible.
