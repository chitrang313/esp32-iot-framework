# DeviceStateManager - instruction-part1.md

# Part 1 - Foundation & Architecture

---

# Objective

Develop a reusable, production-quality DeviceStateManager for the ESP32 IoT Framework.

DeviceStateManager is the single source of truth for the current runtime state of the device.

It stores the latest state of every device component while the system is running.

Unlike PreferencesManager, DeviceStateManager does NOT provide persistent storage.

All data is stored in RAM only.

---

# Deliverables

Create only these files.

```text
Core/
└── DeviceStateManager/
    ├── DeviceStateManager.h
    ├── DeviceStateManager.cpp
    ├── README.md
    ├── instruction-part1.md
    ├── instruction-part2.md
    ├── instruction-part3.md
    ├── instruction-part4.md
    └── instruction-part5.md
```

Do not create additional public headers.

---

# Purpose

DeviceStateManager represents the current state of the device.

Examples

* Relay States
* Switch States
* WiFi State
* Firebase State
* Device Online Status
* Last Synchronization Status
* Uptime
* Device Mode

This information exists only while the device is powered on.

---

# RAM vs Flash

DeviceStateManager

↓

RAM

Temporary

Lost after reboot

PreferencesManager

↓

Flash

Persistent

Survives reboot

Never mix these responsibilities.

---

# Responsibilities

DeviceStateManager is responsible for

* Holding runtime state
* Updating runtime values
* Providing current values
* Tracking state changes
* Providing a single source of truth

DeviceStateManager is NOT responsible for

* Relay control
* WiFi connection
* Firebase communication
* Persistent storage
* Scheduling
* Synchronization

Those belong to other managers.

---

# Architecture

Every framework manager updates DeviceStateManager.

Example

```text
RelayManager
        │
        ▼
DeviceStateManager
        ▲
        │
SwitchManager

IoTWiFiManager

FirebaseManager
```

Future managers should read the current state from DeviceStateManager instead of querying multiple managers.

---

# Design Philosophy

Application code should never maintain duplicate state.

Bad

```cpp
bool relay1State;
bool relay2State;
bool wifiConnected;
```

Good

```cpp
deviceState.getRelayState(...);

deviceState.getWiFiState();

deviceState.getFirebaseState();
```

DeviceStateManager becomes the single source of truth.

---

# Lifecycle

Follow the framework standard.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

loop() may be empty in Phase 1.

---

# Public API Philosophy

The API should use

```cpp
set...

get...

is...
```

Examples

```cpp
setRelayState();

getRelayState();

setWiFiState();

getWiFiState();

setFirebaseState();

getFirebaseState();
```

Public APIs should never expose internal storage.

---

# State Categories

The architecture should support

Hardware

* Relay
* Switch

Network

* WiFi
* Firebase

System

* Uptime
* Boot Count
* Device Mode

Synchronization

* Last Sync Time
* Sync Pending
* Sync Failed

Implementation may be phased.

---

# Enumerations

Use enum class wherever possible.

Examples

```text
DeviceMode

WiFiState

FirebaseState

RelayState

SwitchState

DeviceStateResult
```

Never use raw integers.

---

# Validation

Validate every public function.

Examples

* Invalid channel
* Invalid state
* Invalid index
* Manager disabled
* Manager not initialized

Never crash.

Return DeviceStateResult where appropriate.

---

# Logging

Never use

* Serial.print()
* Serial.println()

Future logging belongs to LoggerManager.

---

# Memory

All runtime state should remain in RAM.

Avoid dynamic allocation.

Use fixed-size storage wherever possible.

Suitable for continuous operation.

---

# Performance

Access to runtime state should be extremely fast.

Prefer direct memory access over expensive operations.

Never block loop().

---

# Future Compatibility

Architecture should support

* Sensors
* Energy Monitoring
* Temperature
* Humidity
* OTA Status
* MQTT Status
* Cloud Status

without changing the public API.

---

# Final Goal

Every manager updates DeviceStateManager.

Every manager reads current runtime state from DeviceStateManager when needed.

DeviceStateManager becomes the single source of truth for the current state of the entire ESP32 device while it is running.
