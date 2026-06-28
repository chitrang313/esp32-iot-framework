# DeviceStateManager - instruction-part2.md

# Part 2 - Runtime State API & State Management

---

# Objective

This part defines how runtime state is stored, updated and accessed.

DeviceStateManager provides a centralized in-memory representation of the current device state.

Every manager should update its own state.

Every manager may read the state of other managers.

---

# Design Philosophy

DeviceStateManager stores runtime state only.

It does not control hardware.

It does not communicate with external systems.

It only stores and provides the current state.

---

# Ownership Rule

Each manager owns its own state.

Examples

```text
IoTWiFiManager
    │
    └── Updates WiFi State

FirebaseManager
    │
    └── Updates Firebase State

RelayManager
    │
    └── Updates Relay State

SwitchManager
    │
    └── Updates Switch State
```

Other managers should not overwrite another manager's state.

---

# Public API

Expose only the following categories.

Lifecycle

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Relay

```cpp
setRelayState();

getRelayState();
```

Switch

```cpp
setSwitchState();

getSwitchState();
```

WiFi

```cpp
setWiFiState();

getWiFiState();
```

Firebase

```cpp
setFirebaseState();

getFirebaseState();
```

Device

```cpp
setDeviceMode();

getDeviceMode();

setOnline();

isOnline();
```

System

```cpp
getUptime();

reset();
```

---

# Relay State

Example

```cpp
deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);

RelayState state =
    deviceState.getRelayState(
        RelayChannel::Relay1
    );
```

---

# Switch State

Example

```cpp
deviceState.setSwitchState(
    SwitchChannel::Switch1,
    SwitchState::Pressed
);

SwitchState state =
    deviceState.getSwitchState(
        SwitchChannel::Switch1
    );
```

---

# WiFi State

Example

```cpp
deviceState.setWiFiState(
    WiFiState::Connected
);

WiFiState state =
    deviceState.getWiFiState();
```

---

# Firebase State

Example

```cpp
deviceState.setFirebaseState(
    FirebaseState::Ready
);

FirebaseState state =
    deviceState.getFirebaseState();
```

---

# Device Mode

Architecture should support

```text
Booting

Normal

Maintenance

SafeMode

Updating
```

Example

```cpp
deviceState.setDeviceMode(
    DeviceMode::Normal
);
```

---

# Online State

Example

```cpp
deviceState.setOnline(
    true
);

bool online =
    deviceState.isOnline();
```

Online means

* WiFi Connected
* Firebase Ready

Implementation is optional.

---

# Uptime

Architecture should provide

```cpp
uint32_t uptime =
    deviceState.getUptime();
```

Implementation should use

```cpp
millis();
```

---

# Reset

Architecture should support

```cpp
deviceState.reset();
```

Reset only runtime state.

Never erase flash storage.

---

# Validation

Every setter validates

* Manager initialized
* Manager enabled
* Valid channel
* Valid state

Every getter validates

* Valid channel
* Valid index

Return safe defaults when necessary.

---

# State Change Detection

When a value changes

↓

Update internal state.

If the value has not changed

↓

Do nothing.

Avoid unnecessary updates.

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

The second call should detect no change.

No unnecessary processing should occur.

---

# Memory Layout

Keep runtime state in fixed-size structures.

Avoid dynamic allocation.

Suitable for long-running embedded systems.

---

# Performance

Getters should execute in constant time.

Setters should update memory directly.

Never block

```cpp
loop();
```

---

# Future Compatibility

Architecture should support additional runtime states.

Examples

* MQTT State
* OTA State
* Temperature
* Humidity
* Power Consumption
* Battery Level
* Diagnostics

without changing the public API.

---

# Final Goal

Every manager updates only its own runtime state.

Every manager can read the latest runtime state from DeviceStateManager.

DeviceStateManager becomes the single in-memory representation of the entire device while it is running.
