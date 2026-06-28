# FirebaseManager - instruction-part5.md

# Part 5 - Framework Standards, Coding Standards & Future Roadmap

---

# Objective

This document defines the implementation standards for FirebaseManager.

These rules ensure the module remains maintainable, scalable and reusable across all future ESP32 projects.

Every future contributor should follow these standards.

---

# Framework Philosophy

FirebaseManager is a framework module.

It is **not** an application.

It must never contain application-specific logic.

Bad Example

```cpp
if(path == "/home/kitchen/light")
{
    relay.turnOn(...);
}
```

Good Example

```cpp
Publish Event

↓

Application

↓

RelayManager
```

---

# Module Independence

FirebaseManager must never directly communicate with

- RelayManager
- InputManager
- StatusLedManager
- IoTWiFiManager
- PreferencesManager
- OTAUpdateManager
- MQTTManager
- AutomationManager

Managers are independent.

Only the application coordinates them.

---

# Callback Philosophy

Every manager should expose

one

callback registration.

Example

```cpp
firebase.onEvent(...);
```

Avoid

```cpp
onConnected()

onDisconnected()

onRead()

onWrite()

onRetry()

onQueue()
```

A single callback is easier to maintain and extend.

---

# Public API Rules

Public APIs are considered stable.

Do not

- Rename public functions
- Change parameter order
- Change enums
- Remove existing overloads

Future versions should remain backward compatible.

---

# Naming Convention

Classes

```text
PascalCase
```

Example

```text
FirebaseManager
```

Functions

```text
camelCase
```

Example

```text
begin()

configure()

subscribe()

getState()
```

Variables

```text
m_memberVariable
```

Private Helpers

```text
camelCase()
```

Enums

```text
enum class
```

Constants

```text
UPPER_CASE
```

Compile-time constants

```text
constexpr
```

---

# Documentation

Every public function should include

- Brief description
- Parameters
- Return value

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
// Prevent unnecessary Firebase writes to reduce network traffic.
```

Bad

```cpp
// Set value.
```

---

# Error Handling

Never silently ignore errors.

Every failure should

- Return FirebaseResult
- Update FirebaseState when appropriate
- Publish FirebaseEvent when appropriate

---

# Validation

Every public API validates

- Module enabled
- Module configured
- Ready state
- Path
- Data type

Never assume parameters are valid.

---

# Memory Rules

Avoid

- String
- new
- delete
- malloc
- free

Prefer

- constexpr
- Fixed-size arrays
- Stack allocation

Suitable for long-running embedded systems.

---

# Performance Rules

Never block

```cpp
loop();
```

Never use

```cpp
delay();
```

Process work incrementally.

Keep loop execution short.

---

# Thread Safety

The framework assumes the Arduino single-threaded execution model.

Do not introduce unnecessary mutexes or RTOS dependencies in Phase 1.

Future versions may support FreeRTOS if required.

---

# Logging

FirebaseManager must never call

```cpp
Serial.print()

Serial.println()
```

Future logging belongs to LoggerManager.

---

# Testing

Implementation should be easy to test.

Public APIs should remain deterministic.

Avoid hidden side effects.

---

# Internal Architecture

Although only one public class exists,

the implementation should remain logically separated.

Recommended internal components

```text
Configuration

Connection

Authentication

Read

Write

Streaming

Queue

Retry

Validation

Event Dispatcher
```

These are implementation details.

---

# Future Features

The architecture should support

without changing the public API

- Firestore
- Cloud Storage
- Remote Config
- Cloud Functions
- Push Notifications
- Device Presence
- Batch Operations
- Transactions
- Offline Database
- Data Synchronization
- Multiple Firebase Projects
- Multiple User Accounts
- Encryption Layer
- Compression
- Conflict Resolution
- Statistics
- Diagnostics

Implementation is NOT required.

Only the architecture should support future expansion.

---

# Project Structure

```text
Cloud/
└── FirebaseManager/
    ├── FirebaseManager.h
    ├── FirebaseManager.cpp
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

- enum class
- const correctness
- helper functions
- descriptive names
- private members
- no duplicated code
- short methods
- readable code

Favor clarity over cleverness.

---

# Deliverables

Implement only

```text
FirebaseManager.h

FirebaseManager.cpp
```

The implementation must satisfy every requirement defined in

- instruction-part1.md
- instruction-part2.md
- instruction-part3.md
- instruction-part4.md
- instruction-part5.md

---

# Final Goal

The application should never know which Firebase library is used.

Application code should only interact with

```cpp
firebase.configure(...);

firebase.begin();

firebase.loop();

firebase.get(...);

firebase.set(...);

firebase.subscribe(...);

firebase.onEvent(...);

firebase.getState();

firebase.isReady();
```

FirebaseManager becomes the single communication layer between the application and Firebase Realtime Database.

It should be reusable across every ESP32 project without modifying the library itself.
