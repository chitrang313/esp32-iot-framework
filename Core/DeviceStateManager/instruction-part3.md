# DeviceStateManager - instruction-part3.md

# Part 3 - Change Tracking, Dirty Flags & State Versioning

---

# Objective

DeviceStateManager should not only store runtime state.

It should also know

* What changed
* When it changed
* Whether synchronization is required

This allows future managers such as

* SyncManager
* EventBus
* LoggerManager

to work efficiently.

---

# Design Philosophy

Application code should never compare every state manually.

Instead

```text
State Changed

↓

DeviceStateManager detects change

↓

Marks state as dirty

↓

Increments version

↓

Future managers react
```

---

# State Change Detection

Every setter should compare

Old Value

↓

New Value

If identical

↓

Do nothing.

If different

↓

Update value.

Mark state as changed.

Increment version.

---

# Dirty Flags

Every state category should maintain its own dirty flag.

Examples

```text
Relay Dirty

Switch Dirty

WiFi Dirty

Firebase Dirty

System Dirty
```

Dirty means

"The value has changed since it was last synchronized."

---

# Public API

Architecture should expose

```cpp
isRelayDirty();

isSwitchDirty();

isWiFiDirty();

isFirebaseDirty();

isSystemDirty();
```

Return

```cpp
bool
```

---

# Clear Dirty Flags

Architecture should expose

```cpp
clearRelayDirty();

clearSwitchDirty();

clearWiFiDirty();

clearFirebaseDirty();

clearSystemDirty();

clearAllDirty();
```

Normally called by

* SyncManager
* EventBus

after successful processing.

---

# State Version

Maintain one global version counter.

Example

```cpp
uint32_t version =
    deviceState.getVersion();
```

Every successful state change

↓

Increment version.

Example

```text
Version 1

↓

Relay Changed

↓

Version 2

↓

WiFi Connected

↓

Version 3
```

---

# Category Version

Future architecture should support

```cpp
getRelayVersion();

getSwitchVersion();

getWiFiVersion();

getFirebaseVersion();
```

Implementation optional.

Global version is sufficient for Phase 1.

---

# Last Update Time

Every state category should support

```cpp
getLastRelayUpdate();

getLastSwitchUpdate();

getLastWiFiUpdate();

getLastFirebaseUpdate();
```

Return

```cpp
uint32_t
```

using

```cpp
millis()
```

Useful for

* Diagnostics
* Timeout detection
* Synchronization

Implementation optional.

---

# Change Counter

Architecture should support

```cpp
getRelayChangeCount();

getSwitchChangeCount();

getWiFiChangeCount();

getFirebaseChangeCount();
```

Useful for diagnostics.

Implementation optional.

---

# Duplicate Protection

Example

```cpp
deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);

deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);
```

The second call

↓

Should NOT

* Increment version
* Set dirty flag
* Update timestamp

Nothing changed.

---

# Synchronization Support

Future SyncManager should work like

```text
Check Dirty Flag

↓

Synchronize

↓

Success

↓

Clear Dirty Flag
```

DeviceStateManager should never perform synchronization itself.

---

# Event Support

Future EventBus should work like

```text
Relay Changed

↓

DeviceStateManager

↓

EventBus publishes event
```

DeviceStateManager itself should not publish events.

---

# Thread Model

Assume Arduino single-threaded execution.

No mutexes.

No RTOS synchronization.

---

# Performance

All change detection should be constant time.

Never scan all states unnecessarily.

Use direct memory comparison.

---

# Memory

Dirty flags

↓

bool

Version

↓

uint32_t

Timestamps

↓

uint32_t

Avoid dynamic allocation.

---

# Future Compatibility

Architecture should support

* Sensor Dirty Flags
* OTA Dirty Flag
* MQTT Dirty Flag
* Cloud Dirty Flag
* Custom State Categories

without changing the public API.

---

# Final Goal

DeviceStateManager should always know

* Current value
* Previous value (if needed internally)
* Whether the value changed
* Whether synchronization is required
* Current state version

Future managers should simply ask

```cpp
if(deviceState.isRelayDirty())
{
    ...
}
```

instead of comparing values themselves.

DeviceStateManager becomes the central runtime state tracker for the entire framework.
