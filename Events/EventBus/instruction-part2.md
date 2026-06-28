# EventBus - instruction-part2.md

# Part 2 - Event API, Subscription Model & Event Flow

---

# Objective

This part defines how modules publish events, subscribe to events and receive notifications.

EventBus acts as the communication hub for the entire framework.

---

# Design Philosophy

Modules never communicate directly.

Instead

```text
Publisher

↓

EventBus

↓

Subscribers
```

The publisher never knows who receives the event.

Subscribers never know who published the event.

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

isIdle();
```

---

# Publish Flow

Example

```text
RelayManager

↓

publish()

↓

Queue Event

↓

loop()

↓

Dispatch

↓

Subscribers
```

Publishing should return immediately.

Never execute callbacks directly inside publish().

---

# Subscribe Flow

Example

```cpp
eventBus.subscribe(
    EventType::RelayStateChanged,
    relayChangedCallback
);
```

One event may have multiple subscribers.

---

# Unsubscribe

Architecture should support

```cpp
unsubscribe();

unsubscribeAll();
```

Subscribers should be removable safely at runtime.

---

# Multiple Subscribers

Example

```text
Relay Changed

↓

EventBus

├── Logger

├── SyncManager

├── Scheduler

└── Application
```

Every subscriber receives the same event.

---

# Event Payload

Each event should contain

```text
Event Type

Event Source

Timestamp

Optional Payload Pointer

Payload Size
```

Payload ownership remains with the publisher unless documented otherwise.

---

# Event Types

Architecture should support

Hardware

```text
RelayChanged

SwitchPressed

SwitchReleased
```

Network

```text
WiFiConnected

WiFiDisconnected
```

Cloud

```text
FirebaseReady

FirebaseDisconnected
```

Synchronization

```text
SyncStarted

SyncCompleted

SyncFailed
```

System

```text
BootCompleted

RestartRequested

ConfigurationChanged
```

Future modules may introduce additional event types.

---

# Event Source

Architecture should identify who generated the event.

Examples

```text
RelayManager

SwitchManager

IoTWiFiManager

FirebaseManager

SyncManager

Application
```

Use enum class.

---

# Event Delivery

Events should be delivered in FIFO order.

First Published

↓

First Delivered

Never reorder events.

---

# Queue Processing

Process queued events only inside

```cpp
loop();
```

Never process events immediately inside

```cpp
publish();
```

---

# Queue Capacity

Use a fixed-size queue.

When full

Return

```text
EventResult::QueueFull
```

Do not dynamically allocate memory.

---

# Validation

Before publishing verify

* Manager initialized
* Manager enabled
* Valid event type
* Queue has space

Before subscribing verify

* Valid callback
* Valid event type

Never crash.

---

# Performance

Publishing should be constant time.

Subscription should be lightweight.

Dispatch should process only a limited number of events per

```cpp
loop();
```

Never block execution.

---

# Future Compatibility

Architecture should support

* Wildcard subscriptions
* Event filtering
* Event priorities
* Delayed events
* Broadcast events
* Remote events

without changing the public API.

---

# Final Goal

Every module should communicate through EventBus.

Publishers know nothing about subscribers.

Subscribers know nothing about publishers.

The EventBus becomes the central communication layer for the ESP32 IoT Framework.
