# SyncManager - instruction-part3.md

# Part 3 - Reliability, Queue Management & Conflict Prevention

---

# Objective

SyncManager must reliably synchronize runtime state under unstable network conditions.

The manager should guarantee that no runtime state changes are lost whenever possible.

Synchronization should continue automatically when connectivity is restored.

---

# Design Philosophy

Synchronization should never depend on perfect network conditions.

The framework must tolerate

* WiFi disconnects
* Firebase reconnects
* Temporary Internet failures
* Cloud timeouts

without losing runtime state.

---

# Synchronization Queue

Architecture should maintain an internal synchronization queue.

Example

```text
Relay1 Changed

↓

Queue

↓

Synchronize

↓

Success

↓

Remove From Queue
```

Only unsynchronized changes should remain in the queue.

---

# Queue Rules

Each queued item should contain

* Item Type
* Current Value
* Synchronization Direction
* Retry Count
* Timestamp
* Synchronization Status

Future implementations may extend this structure.

---

# Duplicate Protection

Never queue identical updates.

Example

```text
Relay1 = ON

↓

Queued

↓

Relay1 = ON

↓

Ignore
```

Only the newest value should remain.

This minimizes unnecessary cloud traffic.

---

# Conflict Prevention

Avoid synchronization loops.

Bad

```text
Firebase

↓

Relay

↓

DeviceState

↓

Sync

↓

Firebase
```

Good

```text
Firebase

↓

Relay

↓

DeviceState

↓

Clear Dirty Flag

↓

No Upload
```

SyncManager must distinguish between

* Local changes
* Remote changes

---

# Retry Strategy

Failed synchronizations should be retried automatically.

Recommended

```text
Attempt 1

↓

Wait

↓

Attempt 2

↓

Wait Longer

↓

Attempt 3
```

Support exponential backoff.

Never retry continuously.

---

# Retry Limits

Architecture should support

```cpp
setMaxRetryCount();
```

and

```cpp
getRetryCount();
```

Implementation optional.

---

# Queue Recovery

If synchronization fails

↓

Keep the item in the queue.

Do not discard pending changes.

---

# WiFi Recovery

When WiFi reconnects

↓

Resume synchronization automatically.

Application code should not manually restart synchronization.

---

# Firebase Recovery

When Firebase reconnects

↓

Resume queued synchronization automatically.

---

# Queue Overflow

Architecture should support

```text
Queue Full

↓

Reject New Item

or

Replace Oldest Item
```

The strategy should be configurable in future versions.

---

# Synchronization Timeout

Architecture should support timeouts.

Example

```text
Cloud Request

↓

Timeout

↓

Retry Later
```

Never block indefinitely.

---

# Synchronization Statistics

Architecture should expose

```cpp
getQueueSize();

getRetryCount();

getSuccessfulSyncCount();

getFailedSyncCount();

getDroppedSyncCount();
```

Implementation optional.

---

# Error Recovery

Common recovery scenarios

* WiFi Lost
* Firebase Lost
* Queue Overflow
* Cloud Timeout
* Invalid Runtime State

SyncManager should recover automatically whenever possible.

---

# Performance

Process only a small number of queue items during each

```cpp
loop();
```

Never process the entire queue in one iteration.

Avoid blocking the application.

---

# Memory

Use fixed-size queues.

Avoid

* malloc
* free
* new
* delete

Suitable for long-running embedded systems.

---

# Future Compatibility

Architecture should support

* Multiple synchronization queues
* Multiple cloud providers
* Prioritized synchronization
* Batch synchronization
* Transaction synchronization

without changing the public API.

---

# Final Goal

SyncManager should reliably synchronize runtime state regardless of temporary connectivity problems.

The application should never need to manually recover synchronization.

All retry logic, queue management, conflict prevention and recovery should be handled internally by SyncManager.
