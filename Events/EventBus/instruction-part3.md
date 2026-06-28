# EventBus - instruction-part3.md

# Part 3 - Event Dispatch, Queue Management & Reliability

---

# Objective

Design a reliable event dispatch engine suitable for long-running ESP32 applications.

Event publishing should always be fast.

Event processing should occur asynchronously inside the EventBus loop.

---

# Design Philosophy

Publishing an event should never immediately execute subscribers.

Instead

```text id="rj67ga"
Publish

↓

Queue Event

↓

loop()

↓

Dispatch

↓

Subscribers
```

This prevents

* Recursive callbacks
* Stack overflows
* Long blocking operations

---

# Event Queue

Maintain an internal fixed-size event queue.

Example

```text id="oqdk1e"
Relay Changed

↓

Queue

↓

Waiting

↓

Dispatch

↓

Remove
```

Events remain in the queue until processed.

---

# Queue Rules

Each queue item should contain

* Event Type
* Event Source
* Timestamp
* Payload Pointer
* Payload Size
* Priority (future-ready)

Do not allocate memory dynamically.

---

# FIFO Processing

Events must be processed in the order they were published.

Example

```text id="ym7k95"
Event A

↓

Event B

↓

Event C
```

Dispatch order

```text id="p2gj0d"
A

↓

B

↓

C
```

Never reorder events.

---

# Multiple Subscribers

Example

```text id="iqy9pm"
Relay Changed

↓

EventBus

├── Logger

├── SyncManager

├── Scheduler

└── Application
```

Each subscriber should receive the event exactly once.

---

# Callback Execution

Callbacks should execute sequentially.

Example

```text id="w9ncb4"
Subscriber 1

↓

Subscriber 2

↓

Subscriber 3
```

Do not execute callbacks in parallel.

---

# Queue Overflow

If the queue becomes full

Return

```text id="mjlwmn"
EventResult::QueueFull
```

Never overwrite existing events automatically.

Never allocate additional memory.

---

# Subscriber Limits

Use a fixed-size subscriber table.

Future configuration may change the maximum.

Implementation should avoid heap allocation.

---

# Slow Subscribers

One slow subscriber must not prevent future architecture improvements.

The framework should encourage subscribers to perform minimal work.

Long-running tasks should be deferred.

---

# Event Dropping

Architecture should expose

```cpp id="9xjlwm"
getDroppedEventCount();
```

Implementation optional.

Useful for diagnostics.

---

# Dispatch Statistics

Architecture should support

```cpp id="gyx8lh"
getPublishedCount();

getDispatchedCount();

getDroppedCount();
```

Implementation optional.

---

# Event Timestamp

Each event should record

```cpp id="mr1vrj"
millis()
```

when it is published.

Useful for

* Diagnostics
* Timeout detection
* Event ordering

---

# Event Lifetime

Payload remains valid only during callback execution unless otherwise documented.

Subscribers must not store raw payload pointers.

If persistence is required, subscribers should copy the data.

---

# Validation

Before dispatch

Verify

* Event exists
* Subscriber exists
* Callback is valid

Skip invalid subscribers safely.

Never crash.

---

# Error Isolation

One subscriber failure should not prevent remaining subscribers from receiving the event.

The dispatch engine should continue processing.

---

# Performance

Publishing should remain constant time.

Dispatch should process only a limited number of events during each

```cpp id="j89ozx"
loop();
```

Never process the entire queue if many events are pending.

Maintain responsive application execution.

---

# Memory

Use

* Fixed-size event queue
* Fixed-size subscriber table

Avoid

* new
* delete
* malloc
* free

Suitable for continuous embedded operation.

---

# Future Compatibility

Architecture should support

* Event priorities
* Delayed events
* Sticky events
* Broadcast events
* Remote events
* Event persistence

without changing the public API.

---

# Final Goal

EventBus should reliably deliver events from publishers to subscribers.

Publishing should always be lightweight.

Dispatch should be deterministic, FIFO-based, non-blocking, and suitable for 24/7 ESP32 operation.

Subscribers should remain independent of each other, ensuring loose coupling across the framework.
