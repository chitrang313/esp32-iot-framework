# SyncManager - instruction-part1.md

# Part 1 - Foundation & Architecture

---

# Objective

Develop a reusable, production-quality SyncManager for the ESP32 IoT Framework.

SyncManager is responsible for synchronizing runtime state between the ESP32 and cloud services.

It does not own any state.

It coordinates synchronization only.

---

# Deliverables

Create only these files.

```text
Core/
└── SyncManager/
    ├── SyncManager.h
    ├── SyncManager.cpp
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

SyncManager keeps runtime state synchronized.

Examples

* Relay State
* Switch State
* Device Status
* WiFi Status
* Cloud Status

Future

* Sensors
* Energy Monitoring
* OTA Status

---

# Responsibilities

SyncManager is responsible for

* Detecting dirty runtime state
* Uploading local changes
* Processing cloud changes
* Clearing dirty flags after successful synchronization
* Preventing synchronization loops
* Retrying failed synchronizations

SyncManager is NOT responsible for

* Relay control
* WiFi connection
* Firebase communication
* Runtime state storage
* Persistent storage
* Logging

---

# Architecture

```text
RelayManager
        │
SwitchManager
        │
IoTWiFiManager
        │
FirebaseManager
        │
        ▼
DeviceStateManager
        │
        ▼
SyncManager
        │
        ▼
FirebaseManager
```

SyncManager reads runtime state.

SyncManager requests synchronization.

Managers remain independent.

---

# Design Philosophy

SyncManager never decides

what the state should be.

It only synchronizes

the current state.

The source of truth remains

DeviceStateManager.

---

# Ownership

RelayManager owns

Relay State.

WiFiManager owns

WiFi State.

FirebaseManager owns

Firebase State.

SyncManager owns

Synchronization.

Nothing else.

---

# Lifecycle

Follow framework standards.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Synchronization happens inside

```cpp
loop();
```

Never block execution.

---

# Public API

Expose only

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
```

Information

```cpp
getState();

isBusy();

isIdle();
```

---

# Synchronization Sources

Architecture should support

Local

↓

Cloud

Cloud

↓

Local

without changing the public API.

---

# Synchronization Targets

Current

Firebase

Future

* MQTT
* REST API
* BLE
* Local Gateway

SyncManager should remain transport-independent.

---

# Enumerations

Use enum class.

Examples

```text
SyncState

SyncDirection

SyncResult

SyncPriority
```

Never use raw integers.

---

# Validation

Validate

* Manager initialized
* Manager enabled
* Cloud ready
* Runtime state available

Return SyncResult.

Never crash.

---

# Logging

Never use

* Serial.print()
* Serial.println()

Future logging belongs to LoggerManager.

---

# Memory

Avoid

* Dynamic allocation
* Heap fragmentation

Prefer fixed-size structures.

---

# Performance

Synchronization must be incremental.

Never block loop().

Never synchronize everything every iteration.

---

# Future Compatibility

Architecture should support

* Multiple cloud providers
* Multiple synchronization targets
* Offline synchronization
* Conflict resolution
* Queue-based synchronization

without changing the public API.

---

# Final Goal

SyncManager becomes the synchronization engine of the framework.

It reads runtime state from DeviceStateManager.

It requests cloud updates through FirebaseManager.

It never owns runtime state.

It never owns persistent storage.

It coordinates synchronization only.
