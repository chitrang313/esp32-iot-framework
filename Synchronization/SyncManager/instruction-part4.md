# SyncManager - instruction-part4.md

# Part 4 - Multi-Target Synchronization, Statistics & Future Extensibility

---

# Objective

Design SyncManager so it can synchronize runtime state with multiple destinations while keeping a stable public API.

The application should never know how synchronization is performed internally.

---

# Design Philosophy

Today

```text
DeviceStateManager
        │
        ▼
SyncManager
        │
        ▼
FirebaseManager
```

Future

```text
                 DeviceStateManager
                         │
                         ▼
                  SyncManager
        ┌────────┼────────┬─────────┐
        ▼        ▼        ▼         ▼
 Firebase   MQTTManager   REST    Matter
```

The application should not change when new synchronization targets are added.

---

# Multiple Synchronization Targets

The architecture should support

* Firebase
* MQTT
* REST API
* BLE
* Matter
* Thread
* Local Gateway

Implementation is not required in Phase 1.

---

# Synchronization Targets

Future architecture should allow

```cpp
syncManager.enableTarget(...);

syncManager.disableTarget(...);
```

Application chooses which cloud services are active.

Implementation optional.

---

# Selective Synchronization

Architecture should support

Examples

```text
Relay State

↓

Firebase Only
```

```text
Diagnostics

↓

MQTT Only
```

```text
Temperature

↓

Firebase + MQTT
```

Not every state must synchronize to every target.

---

# Batch Synchronization

Future architecture should support

```text
Relay1

Relay2

Relay3

↓

One Cloud Request
```

Instead of

```text
Relay1

↓

Cloud

Relay2

↓

Cloud

Relay3

↓

Cloud
```

This reduces network traffic.

Implementation optional.

---

# Synchronization Filters

Future architecture should support

Examples

* Synchronize only relay state
* Synchronize only sensors
* Synchronize only diagnostics

Implementation optional.

---

# Statistics

Architecture should expose

```cpp
getSuccessfulSyncCount();

getFailedSyncCount();

getQueueSize();

getPendingCount();

getDroppedCount();
```

Implementation optional.

---

# Diagnostics

Architecture should support

```cpp
getLastSyncTime();

getLastSyncResult();

getCurrentTarget();

getCurrentQueueSize();
```

Useful for diagnostics and debugging.

---

# Synchronization Health

Architecture should support

```cpp
isHealthy();
```

Health may consider

* Queue Size
* Retry Count
* Last Successful Sync
* Cloud Availability

Implementation optional.

---

# Offline Operation

The framework should continue operating normally when cloud services are unavailable.

Example

```text
Relay

↓

DeviceState

↓

Queue

↓

Wait

↓

Reconnect

↓

Synchronize
```

Application code should not require special handling.

---

# Future Cloud Providers

Adding another cloud provider should not require

* Changing SyncManager public API
* Changing DeviceStateManager
* Changing RelayManager
* Changing SwitchManager

Only the synchronization backend should change.

---

# Performance

Only synchronize required data.

Avoid

* Duplicate uploads
* Unnecessary cloud requests
* Large batches during one loop()

Never block execution.

---

# Memory

Use fixed-size queues.

Avoid dynamic allocation.

Suitable for continuous operation.

---

# Future Compatibility

Architecture should support

* Multiple devices
* Device groups
* Peer-to-peer synchronization
* Cloud failover
* Cloud redundancy
* Synchronization policies

without changing the public API.

---

# Final Goal

SyncManager becomes the framework's synchronization engine.

It should support multiple synchronization targets, offline operation, diagnostics, statistics, and future cloud providers while presenting one simple, stable API to the application.

The application should never know where data is being synchronized.
