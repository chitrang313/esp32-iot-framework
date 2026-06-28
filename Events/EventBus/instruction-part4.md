# EventBus - instruction-part4.md

# Part 4 - Event Filtering, Diagnostics & Future Extensibility

---

# Objective

Design EventBus so it can support future framework features without requiring public API changes.

The application should continue using the same publish/subscribe interface regardless of future capabilities.

---

# Design Philosophy

Today

```text id="s5dhj9"
Publisher

↓

EventBus

↓

Subscribers
```

Future

```text id="0kkjlwm"
Publisher

↓

EventBus

↓

Filter

↓

Priority

↓

Delayed Queue

↓

Subscribers
```

The public API should remain unchanged.

---

# Event Filtering

Future architecture should support

Examples

```text id="5s9g5q"
Logger

↓

Receive All Events
```

```text id="yd93wr"
SyncManager

↓

Receive Only Synchronization Events
```

```text id="9o3m4q"
Application

↓

Receive Relay Events Only
```

Implementation optional.

---

# Wildcard Subscription

Architecture should support

```cpp id="jwsq3y"
subscribeAll(...);
```

One subscriber receives every event.

Useful for

* Logger
* Diagnostics
* Debugging

Implementation optional.

---

# Event Priority

Architecture should support

```text id="mny8ea"
High

Normal

Low
```

Examples

High

* Emergency Stop
* Restart Requested

Normal

* Relay Changed
* WiFi Connected

Low

* Diagnostics
* Statistics

Implementation optional.

---

# Delayed Events

Future architecture should support

```text id="wbqgpt"
Publish

↓

Wait

↓

Dispatch
```

Useful for

* Retry operations
* Delayed actions
* Scheduled notifications

Implementation optional.

---

# Sticky Events

Architecture should support

```text id="pqctsj"
Subscriber Joins

↓

Immediately Receives

↓

Latest Event
```

Useful for

* Current WiFi State
* Current Device Mode
* Current Cloud State

Implementation optional.

---

# Broadcast Events

Future architecture should support

```text id="p4zjlwm"
Publish

↓

Every Subscriber
```

without explicitly subscribing to individual event types.

Implementation optional.

---

# Event Statistics

Architecture should expose

```cpp id="tzww08"
getPublishedCount();

getDispatchedCount();

getDroppedCount();

getSubscriberCount();

getQueueSize();
```

Useful for diagnostics.

Implementation optional.

---

# Event Diagnostics

Architecture should support

```cpp id="ddylcf"
getLastPublishedEvent();

getLastDispatchedEvent();

getLastPublisher();

getLastDispatchTime();
```

Useful for debugging.

Implementation optional.

---

# Event Health

Architecture should support

```cpp id="xh9u4u"
isHealthy();
```

Health may consider

* Queue Usage
* Dropped Events
* Subscriber Count
* Dispatch Errors

Implementation optional.

---

# Queue Monitoring

Architecture should support

```cpp id="mbg99j"
getQueueUsage();
```

Returns current queue utilization.

Useful for tuning queue size.

---

# Future Framework Integration

EventBus should integrate naturally with

* LoggerManager
* Scheduler
* SyncManager
* OTA Manager
* MQTT Manager
* Diagnostics Manager

without changing its public API.

---

# Performance

Avoid unnecessary event copies.

Avoid duplicate event dispatch.

Dispatch only pending events.

Never block execution.

---

# Memory

Use fixed-size storage.

Avoid

* malloc
* free
* new
* delete

Suitable for long-running embedded systems.

---

# Future Compatibility

Architecture should support

* Event persistence
* Remote events
* Distributed events
* Multiple event queues
* Event replay
* Event recording

without changing the public API.

---

# Final Goal

EventBus becomes the communication backbone of the ESP32 IoT Framework.

It should support diagnostics, filtering, priorities, and future extensions while maintaining one simple publish/subscribe interface.

The application and framework modules should never need to know how events are internally dispatched.
