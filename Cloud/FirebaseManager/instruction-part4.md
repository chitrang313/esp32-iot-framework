# FirebaseManager - instruction-part4.md

# Part 4 - Reliability, Offline Queue & Recovery Engine

---

# Objective

FirebaseManager must continue operating reliably even when

- WiFi disconnects
- Firebase becomes unavailable
- Internet is unstable
- Authentication expires
- Temporary server failures occur

Application code should never implement retry logic.

FirebaseManager owns all recovery mechanisms.

---

# Design Philosophy

The application writes

```cpp
firebase.set(...);
```

FirebaseManager decides

- Send immediately
- Queue operation
- Retry later
- Reject request

The application should not know which strategy was used.

---

# Offline Queue

Support an internal operation queue.

The queue stores Firebase operations that cannot be executed immediately.

Examples

- Write Boolean
- Write Integer
- Write Float
- Write String
- Write JSON

The queue should be completely internal.

---

# Queue Rules

Queue operations only when

- WiFi unavailable
- Firebase disconnected
- Authentication pending
- Retry delay active

Do NOT queue

- Invalid paths
- Invalid values
- Configuration errors
- Authentication failures caused by invalid credentials

These should fail immediately.

---

# Queue Capacity

Provide a configurable compile-time limit.

Example

```cpp
MAX_QUEUE_OPERATIONS
```

Default

```text
50
```

If the queue becomes full

Return

```text
FirebaseResult::QueueFull
```

Do not overwrite existing operations.

---

# Queue Order

Maintain FIFO order.

Example

```text
Operation 1

↓

Operation 2

↓

Operation 3
```

The oldest operation executes first.

---

# Queue Processing

When Firebase becomes Ready

↓

Automatically begin processing the queue.

Application code should never call

```cpp
flushQueue();
```

Queue flushing is automatic.

---

# Queue Events

Publish events

```text
QueueStarted

QueueFlushed

QueueEmpty

QueueFull

QueueOperationFailed
```

The application may observe these events.

---

# Retry Engine

Failed operations should not immediately retry forever.

Use a retry engine.

Example

```text
Attempt 1

↓

Delay

↓

Attempt 2

↓

Delay

↓

Attempt 3

↓

Longer Delay

↓

Attempt 4
```

---

# Retry Policy

Retry only temporary failures.

Examples

Retry

- Network Error
- Timeout
- Temporary Firebase Failure

Do NOT Retry

- Invalid Credentials
- Invalid Path
- Invalid Data Type
- Permission Denied
- Invalid Configuration

---

# Exponential Backoff

The retry delay should increase automatically.

Concept

```text
1 Second

↓

2 Seconds

↓

4 Seconds

↓

8 Seconds

↓

16 Seconds
```

Do not reconnect aggressively.

---

# Retry Limit

Support a configurable retry limit.

Example

```cpp
MAX_RETRY_COUNT
```

When exceeded

Publish

```text
RetryLimitReached
```

Stop retrying until

- WiFi changes
- Connection changes
- Application reconfigures Firebase

---

# Operation Timeout

Every Firebase operation should have a timeout.

Never wait forever.

Timeout should

- Cancel operation
- Publish Timeout event
- Retry if appropriate

---

# Duplicate Protection

Avoid duplicate operations.

Example

```cpp
firebase.set(
    "/relay/1",
    true
);

firebase.set(
    "/relay/1",
    true
);
```

The implementation may detect duplicates to reduce unnecessary traffic.

Architecture should support this.

---

# Queue Validation

Before executing queued operations verify

- WiFi Connected
- Firebase Connected
- Authentication Valid
- Ready State

If not ready

Pause queue.

Do not discard operations.

---

# Queue Persistence

Phase 1

Do NOT save queue to flash.

Architecture should support future persistence using

PreferencesManager.

---

# Memory Management

Queue must avoid

- Dynamic Allocation
- String
- Heap Fragmentation

Prefer fixed-size storage.

Suitable for 24/7 devices.

---

# Error Recovery

FirebaseManager should automatically recover from

- WiFi Loss
- Temporary Firebase Errors
- Temporary Authentication Errors
- Token Refresh
- Network Timeout

Application code should not restart the manager.

---

# Power Recovery

Future architecture should support

- Restoring queued operations
- Restoring subscriptions
- Restoring internal state

after reboot.

Do not implement.

---

# Connection Recovery

When WiFi returns

↓

Firebase reconnects

↓

Authenticate

↓

Restore subscriptions

↓

Flush queue

↓

Publish Ready event

No application code required.

---

# Statistics

Architecture should support future statistics.

Examples

- Successful Reads
- Successful Writes
- Failed Writes
- Retry Count
- Queue Size
- Queue Peak Size
- Connection Count
- Authentication Count

Do not implement.

---

# Diagnostics

Architecture should support future diagnostics.

Examples

- Last Error
- Last Successful Connection
- Last Retry Time
- Last Queue Flush
- Average Response Time

Do not implement.

---

# Performance

Never block

```cpp
loop();
```

Process queue incrementally.

Avoid long execution times.

The manager should remain responsive.

---

# Validation

Every queued operation must be validated before execution.

Never execute invalid operations.

Never corrupt the queue.

---

# Future Compatibility

Architecture should support

- Batch Writes
- Transactions
- Multi-path Updates
- Synchronization Engine
- Offline Database
- Conflict Resolution

without changing the public API.

---

# Final Goal

The application should simply write

```cpp
firebase.set(...);
```

If online

↓

Send immediately.

If offline

↓

Queue automatically.

When WiFi returns

↓

Reconnect.

↓

Authenticate.

↓

Restore subscriptions.

↓

Flush queue.

↓

Continue normal operation.

The application should never know whether Firebase was temporarily unavailable.

FirebaseManager becomes a self-healing communication layer for the entire IoT framework.
