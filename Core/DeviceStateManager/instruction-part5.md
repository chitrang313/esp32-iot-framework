# DeviceStateManager - instruction-part5.md

# Part 5 - Framework Standards, Coding Standards & Future Roadmap

---

# Objective

This document defines the implementation standards for DeviceStateManager.

Every implementation must follow these rules to ensure the framework remains consistent, reusable and maintainable.

---

# Framework Philosophy

DeviceStateManager is a framework module.

It is **not** an application.

It must never contain application-specific logic.

Bad Example

```cpp
deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);

// Immediately update Firebase
firebase.set(...);
```

Good Example

```cpp
deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);
```

DeviceStateManager stores runtime state only.

---

# Module Independence

DeviceStateManager must never directly communicate with

* IoTWiFiManager
* RelayManager
* SwitchManager
* FirebaseManager
* PreferencesManager
* SyncManager
* Scheduler
* LoggerManager
* EventBus

Every manager remains independent.

Only the application coordinates managers.

---

# Ownership Rules

Every runtime state has one owner.

Examples

```text
Relay State
    Owner → RelayManager

Switch State
    Owner → SwitchManager

WiFi State
    Owner → IoTWiFiManager

Firebase State
    Owner → FirebaseManager

Device Mode
    Owner → Application
```

DeviceStateManager never decides ownership.

It only stores the latest state.

---

# Public API Rules

The public API is considered stable.

Do NOT

* Rename public functions
* Change parameter order
* Remove overloads
* Remove enums
* Change return values

Maintain backward compatibility.

---

# Naming Convention

Classes

```text
PascalCase
```

Example

```text
DeviceStateManager
```

Functions

```text
camelCase
```

Examples

```text
setRelayState()

getRelayState()

setWiFiState()

getWiFiState()

getVersion()

clearAllDirty()
```

Private members

```text
m_memberVariable
```

Private helpers

```text
camelCase()
```

Use

```cpp
enum class
```

for every enumeration.

---

# Documentation

Every public function should include

* Brief description
* Parameters
* Return value

Use Doxygen-style comments.

---

# Comment Style

Comments explain

WHY

not

WHAT

Good

```cpp
// Skip update because the state is unchanged.
```

Bad

```cpp
// Set relay state.
```

---

# Validation

Every setter must validate

* Manager initialized
* Manager enabled
* Valid channel
* Valid state

Every getter must validate

* Valid channel
* Valid index

Return safe defaults whenever possible.

Never crash.

---

# Error Handling

Every public function should return

```text
DeviceStateResult
```

where appropriate.

Never silently ignore errors.

Never throw exceptions.

---

# Memory Rules

Store all runtime data in RAM.

Avoid

* new
* delete
* malloc
* free

Prefer

* fixed-size arrays
* stack allocation
* constexpr

Suitable for continuous operation.

---

# Performance Rules

Getters should execute in constant time.

Setters should avoid unnecessary processing.

Never

* scan all runtime data
* allocate memory
* block execution

Avoid

```cpp
delay();
```

---

# Logging

DeviceStateManager must never call

```cpp
Serial.print();

Serial.println();
```

Future logging belongs to LoggerManager.

---

# Thread Model

Assume Arduino single-threaded execution.

Do not introduce

* FreeRTOS Tasks
* Mutexes
* Semaphores

unless explicitly requested.

---

# Internal Organization

Only one public class should exist.

Internally the implementation should be logically organized.

Suggested internal sections

```text
Initialization

Validation

Relay State

Switch State

Network State

System State

Dirty Flags

Versioning

Statistics

Helpers
```

These are implementation details only.

---

# Folder Structure

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

Maintain this structure.

Do not create additional public files.

---

# Coding Standards

Use

* enum class
* const correctness
* helper functions
* descriptive names
* private members
* no duplicated code
* readable code

Prefer clarity over clever implementations.

---

# Future Features

The architecture should support

without changing the public API

* Sensor States
* MQTT State
* OTA State
* Diagnostics
* Runtime Statistics
* Device Snapshot
* Custom Runtime States
* Health Monitoring
* Power Monitoring
* Event Integration

Implementation is NOT required.

Only the architecture should support future expansion.

---

# Deliverables

Implement only

```text
DeviceStateManager.h

DeviceStateManager.cpp
```

The implementation must satisfy every requirement defined in

* instruction-part1.md
* instruction-part2.md
* instruction-part3.md
* instruction-part4.md
* instruction-part5.md

---

# Final Goal

DeviceStateManager becomes the single source of truth for the entire runtime state of the ESP32.

Every manager owns and updates only its own state.

Every manager may read runtime state from DeviceStateManager.

No module should maintain duplicate runtime state.

Future modules such as

* SyncManager
* EventBus
* LoggerManager
* Scheduler

should use DeviceStateManager as the central runtime database.

The application should never duplicate or manually synchronize runtime state.
