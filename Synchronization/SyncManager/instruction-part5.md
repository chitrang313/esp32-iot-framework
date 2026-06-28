# SyncManager - instruction-part5.md

# Part 5 - Framework Standards, Coding Standards & Future Roadmap

---

# Objective

This document defines the implementation standards for SyncManager.

Every implementation must follow these rules to ensure the framework remains modular, reliable, scalable, and reusable.

---

# Framework Philosophy

SyncManager is a framework module.

It is **not** an application.

It must never contain application-specific logic.

Bad Example

```cpp
if (relay1 == ON)
{
    firebase.set("/relay1", true);
}
```

Good Example

```cpp
syncManager.sync();
```

SyncManager coordinates synchronization only.

---

# Module Independence

SyncManager must never directly communicate with

* RelayManager
* SwitchManager
* StatusLedManager
* IoTWiFiManager
* PreferencesManager
* LoggerManager
* Scheduler
* EventBus

SyncManager may communicate only with

* DeviceStateManager
* FirebaseManager

Future cloud providers may also be supported.

---

# Responsibilities

SyncManager owns

* Synchronization Queue
* Retry Logic
* Dirty Flag Processing
* Conflict Prevention
* Synchronization Statistics

SyncManager does NOT own

* Runtime State
* Persistent Storage
* Hardware Control
* Business Logic

---

# Public API Rules

The public API is considered stable.

Do NOT

* Rename public functions
* Change parameter order
* Remove overloads
* Remove enums
* Change return types

Maintain backward compatibility.

---

# Naming Convention

Classes

```text
PascalCase
```

Example

```text
SyncManager
```

Functions

```text
camelCase
```

Examples

```text
begin()

sync()

syncAll()

retrySync()

cancelSync()

getState()

isBusy()
```

Private members

```text
m_memberVariable
```

Private helpers

```text
camelCase()
```

Always use

```cpp
enum class
```

for enumerations.

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
// Retry later to avoid flooding the cloud service.
```

Bad

```cpp
// Retry synchronization.
```

---

# Validation

Before every synchronization verify

* Manager initialized
* Manager enabled
* DeviceStateManager ready
* FirebaseManager ready
* Queue item valid

Never synchronize invalid data.

---

# Error Handling

Every synchronization operation should return

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

QueueFull

Timeout

Failed
```

Never silently ignore failures.

Never throw exceptions.

---

# Memory Rules

Avoid

* malloc
* free
* new
* delete

Prefer

* fixed-size queues
* constexpr
* stack allocation

Suitable for long-running embedded systems.

---

# Performance Rules

Synchronization must be incremental.

Never synchronize the entire queue during one

```cpp
loop();
```

Avoid

* Duplicate uploads
* Duplicate retries
* Busy waiting
* Blocking operations

Never use

```cpp
delay();
```

---

# Logging

SyncManager must never call

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

Suggested internal sections

```text
Initialization

Queue Management

Dirty Flag Processing

Retry Engine

Conflict Detection

Synchronization Engine

Statistics

Validation

Helpers
```

These are implementation details.

---

# Folder Structure

```text
Synchronization/
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

* Multiple cloud providers
* Batch synchronization
* Priority synchronization
* Selective synchronization
* Synchronization policies
* Queue persistence
* Transaction synchronization
* Cloud failover
* Cloud redundancy
* Synchronization diagnostics

Implementation is NOT required.

Only the architecture should support future expansion.

---

# Deliverables

Implement only

```text
SyncManager.h

SyncManager.cpp
```

The implementation must satisfy every requirement defined in

* instruction-part1.md
* instruction-part2.md
* instruction-part3.md
* instruction-part4.md
* instruction-part5.md

---

# Final Goal

SyncManager becomes the synchronization engine of the ESP32 IoT Framework.

It reads runtime state from DeviceStateManager.

It synchronizes changes through FirebaseManager.

It tracks retries, queue status, and synchronization health.

It prevents synchronization loops.

It automatically recovers after temporary network failures.

It never owns runtime state.

It never owns persistent storage.

It never controls hardware.

Its only responsibility is reliable synchronization.
