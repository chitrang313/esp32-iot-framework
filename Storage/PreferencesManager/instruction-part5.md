# PreferencesManager - instruction-part5.md

# Part 5 - Framework Standards, Coding Standards & Future Roadmap

---

# Objective

This document defines the implementation standards for PreferencesManager.

These rules ensure the module remains maintainable, scalable and reusable across all future ESP32 projects.

Every future contributor should follow these standards.

---

# Framework Philosophy

PreferencesManager is a framework module.

It is **not** an application.

It must never contain application-specific logic.

Bad Example

```cpp
preferences.save(
    "livingroom.relay",
    true
);
```

Good Example

```cpp
preferences.save(
    PreferenceKey::Relay1State,
    true
);
```

The application owns the keys.

PreferencesManager owns the storage.

---

# Module Independence

PreferencesManager must never directly communicate with

* IoTWiFiManager
* RelayManager
* SwitchManager
* StatusLedManager
* FirebaseManager
* DeviceStateManager
* Scheduler
* LoggerManager
* EventBus

Managers remain independent.

Only the application coordinates them.

---

# Public API Rules

Public APIs are considered stable.

Do NOT

* Rename public functions
* Change parameter order
* Remove overloads
* Change enums
* Remove return values

Future versions must remain backward compatible.

---

# Naming Convention

Classes

```text
PascalCase
```

Example

```text
PreferencesManager
```

Functions

```text
camelCase
```

Examples

```text
begin()

save()

load()

exists()

remove()

clear()

getState()
```

Variables

```text
m_memberVariable
```

Private helpers

```text
camelCase()
```

Enums

```text
enum class
```

Compile-time constants

```text
constexpr
```

---

# Documentation

Every public function should include

* Brief description
* Parameters
* Return value

Use Doxygen-style comments.

Private helper functions should include comments when helpful.

---

# Comment Style

Comments should explain

WHY

not

WHAT

Good

```cpp
// Skip flash write to reduce flash wear.
```

Bad

```cpp
// Save value.
```

---

# Error Handling

Never silently ignore failures.

Every public function should

* Validate parameters
* Return PreferencesResult
* Update PreferencesState when appropriate

Never throw exceptions.

Never crash.

---

# Validation

Validate

* Initialization
* Enabled state
* Keys
* Values
* Data type
* Null pointers

Reject invalid input immediately.

---

# Memory Rules

Avoid

* String (unless future framework explicitly supports it)
* new
* delete
* malloc
* free

Prefer

* constexpr
* stack allocation
* fixed-size buffers

Suitable for 24/7 embedded systems.

---

# Performance Rules

Avoid unnecessary

* Flash writes
* Reads
* Memory copies

Never use

```cpp
delay();
```

Operations should complete quickly.

---

# Logging

PreferencesManager must never call

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

unless explicitly required.

---

# Internal Architecture

Only one public class exists.

Internally the implementation should remain logically separated.

Recommended internal components

```text
Initialization

Validation

Read

Write

Key Management

Storage Access

State Management
```

These are implementation details.

---

# Folder Structure

```text
Storage/
└── PreferencesManager/
    ├── PreferencesManager.h
    ├── PreferencesManager.cpp
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

Favor clarity over cleverness.

---

# Future Features

The architecture should support

without changing the public API

* Encrypted storage
* Compressed storage
* Binary blobs
* JSON storage
* Multiple storage backends
* Transactions
* Backup & Restore
* Factory Reset
* Cloud synchronization
* Storage diagnostics
* Flash health monitoring

Implementation is NOT required.

Only the architecture should support future expansion.

---

# Deliverables

Implement only

```text
PreferencesManager.h

PreferencesManager.cpp
```

The implementation must satisfy every requirement defined in

* instruction-part1.md
* instruction-part2.md
* instruction-part3.md
* instruction-part4.md
* instruction-part5.md

---

# Final Goal

The application should never interact with the ESP32 Preferences library directly.

Application code should only use

```cpp
preferences.begin();

preferences.loop();

preferences.save(...);

preferences.load(...);

preferences.exists(...);

preferences.remove(...);

preferences.clear();

preferences.getState();

preferences.isReady();
```

PreferencesManager becomes the single storage layer for the entire ESP32 IoT Framework.

Every future manager should rely on PreferencesManager instead of using ESP32 Preferences directly.
