# SyncManager - instruction-part2.md

# Part 2 - Synchronization API & Synchronization Engine

---

# Objective

This part defines how synchronization is performed.

SyncManager coordinates synchronization between DeviceStateManager and cloud services.

It never owns runtime state.

---

# Design Philosophy

DeviceStateManager

↓

Stores runtime state

SyncManager

↓

Synchronizes runtime state

FirebaseManager

↓

Transfers data

Each manager has exactly one responsibility.

---

# Synchronization Flow

Local Change

```text
RelayManager

↓

DeviceStateManager

↓

Dirty Flag

↓

SyncManager

↓

FirebaseManager

↓

Firebase
```

Cloud Change

```text
Firebase

↓

FirebaseManager

↓

DeviceStateManager

↓

SyncManager

↓

Clear Dirty Flag
```

Never create synchronization loops.

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

Synchronization

```cpp
sync();

syncAll();

cancelSync();

retrySync();
```

Information

```cpp
getState();

getLastResult();

isBusy();

isIdle();
```

Statistics

```cpp
getPendingCount();

getSuccessfulSyncCount();

getFailedSyncCount();
```

---

# Synchronization Direction

Architecture should support

```text
Local

↓

Cloud
```

and

```text
Cloud

↓

Local
```

Future synchronization targets should not require API changes.

---

# Dirty Flag Processing

SyncManager checks

```cpp
deviceState.isRelayDirty();

deviceState.isSwitchDirty();

deviceState.isWiFiDirty();
```

If dirty

↓

Synchronize

↓

Success

↓

Clear Dirty Flag

---

# Full Synchronization

Architecture should support

```cpp
syncAll();
```

Example use cases

* Device boot
* WiFi reconnect
* Firebase reconnect
* Manual synchronization

---

# Incremental Synchronization

Normal operation should synchronize

Only Changed Data

Never upload everything.

---

# Synchronization Order

Recommended order

1.

Relay State

2.

Switch State

3.

Device Status

4.

WiFi Status

5.

Other Runtime State

Implementation may evolve.

---

# Retry

Architecture should support

```cpp
retrySync();
```

Retry only failed items.

Never resend successful items.

---

# Cancel

Architecture should support

```cpp
cancelSync();
```

Current synchronization should stop safely.

---

# Synchronization Queue

Architecture should support

```text
Dirty State

↓

Queue

↓

Synchronize

↓

Remove From Queue
```

Implementation may begin with a simple queue.

Future versions may optimize it.

---

# Synchronization Priority

Architecture should support

```text
High

Normal

Low
```

Examples

High

Relay State

Normal

Device Status

Low

Diagnostics

Implementation optional.

---

# Validation

Before synchronizing verify

* Manager initialized
* Manager enabled
* DeviceStateManager available
* FirebaseManager ready

Never synchronize invalid state.

---

# Error Handling

Return

```text
SyncResult
```

Examples

```text
Success

Busy

Cancelled

NoChanges

CloudUnavailable

InvalidState

Failed
```

Never crash.

---

# Performance

Synchronize only when required.

Never block

```cpp
loop();
```

Avoid unnecessary cloud writes.

---

# Memory

Avoid dynamic allocation.

Prefer fixed-size queues.

Suitable for continuous operation.

---

# Future Compatibility

Architecture should support

* MQTT
* REST API
* BLE
* Matter
* Thread

without changing the public API.

---

# Final Goal

SyncManager becomes the synchronization engine of the framework.

It watches DeviceStateManager.

It requests cloud updates through FirebaseManager.

It clears dirty flags after successful synchronization.

It never owns runtime state.

It never communicates directly with hardware managers.
