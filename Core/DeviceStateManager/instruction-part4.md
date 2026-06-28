# DeviceStateManager - instruction-part4.md

# Part 4 - Runtime Database, Statistics & Future Extensibility

---

# Objective

DeviceStateManager should become the central runtime database for the ESP32.

Every manager should publish its current runtime state here.

Future managers should retrieve runtime information from DeviceStateManager instead of querying individual managers.

---

# Design Philosophy

DeviceStateManager stores runtime information only.

It never

* Controls hardware
* Connects to WiFi
* Connects to Firebase
* Saves flash data
* Performs synchronization

It only provides a consistent snapshot of the current device state.

---

# Runtime Database

The architecture should support storing runtime information for future modules.

Examples

Hardware

* Relay States
* Switch States

Network

* WiFi State
* Firebase State
* MQTT State

System

* Device Mode
* Uptime
* Boot Reason
* Reset Reason

Cloud

* Sync Status
* Last Cloud Update

Sensors

* Temperature
* Humidity
* Power Monitoring
* Battery Level

Implementation may be phased.

---

# Device Snapshot

Architecture should support obtaining a complete runtime snapshot.

Example

```cpp
DeviceSnapshot snapshot =
    deviceState.getSnapshot();
```

A snapshot represents the current runtime state of the device.

Implementation is optional in Phase 1.

---

# Runtime Statistics

Architecture should support

```cpp
getRelayChangeCount();

getSwitchChangeCount();

getWiFiReconnectCount();

getFirebaseReconnectCount();

getSyncCount();
```

Useful for diagnostics.

Implementation optional.

---

# Runtime Health

Architecture should support

```cpp
isHealthy();
```

Health may consider

* WiFi Connected
* Firebase Ready
* No synchronization failures
* No runtime errors

Implementation optional.

---

# Runtime Diagnostics

Architecture should support

```cpp
getDiagnostics();
```

Future diagnostics may include

* Current Version
* Dirty Flags
* Last Update Time
* System State
* Runtime Errors

Implementation optional.

---

# Runtime Reset

Architecture should support

```cpp
deviceState.resetRuntime();
```

Behavior

* Clear runtime values
* Reset dirty flags
* Reset version counters

Do NOT erase persistent storage.

PreferencesManager owns persistent data.

---

# Device Online Status

Architecture should support

```cpp
deviceState.isOnline();
```

Online should represent

* WiFi Connected
* Firebase Ready

Future implementations may expand this definition.

---

# Boot Information

Architecture should support

```cpp
getBootTime();

getBootReason();

getResetReason();
```

Useful for diagnostics.

Implementation optional.

---

# Event Readiness

Architecture should expose enough information for future EventBus integration.

Example

```text
Relay Changed

↓

DeviceStateManager

↓

EventBus
```

DeviceStateManager should not publish events itself.

---

# Synchronization Readiness

Architecture should expose enough information for future SyncManager integration.

Example

```text
Dirty Relay

↓

SyncManager reads dirty flag

↓

Synchronize

↓

Clear dirty flag
```

DeviceStateManager never performs synchronization.

---

# Performance

Runtime state access should be constant time.

Avoid scanning all state values.

Avoid unnecessary memory copies.

---

# Memory

Store runtime state in fixed-size structures.

Avoid

* Dynamic allocation
* Heap fragmentation

Suitable for long-running embedded systems.

---

# Future Compatibility

Architecture should support

* OTA State
* Logger State
* Scheduler State
* EventBus State
* User-defined runtime state

without changing the public API.

---

# Final Goal

DeviceStateManager becomes the single runtime database for the ESP32.

Every manager updates its own runtime information.

Every future manager reads runtime information from DeviceStateManager.

The application should never maintain duplicate runtime state.
