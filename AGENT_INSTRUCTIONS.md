# AGENT_INSTRUCTIONS.md

# Objective

This document defines the implementation rules for every module in this ESP32 IoT Framework.

Every coding agent (Claude Code, GitHub Copilot, Cursor, Windsurf, Cline, ChatGPT, etc.) **must read this file first** before implementing any module.

This document applies to every module in the repository.

---

# Required Reading Order

Before writing any code, always read documents in this exact order.

```text
1. AGENT_INSTRUCTIONS.md

2. Module README.md

3. instruction-part1.md
4. instruction-part2.md
5. instruction-part3.md
...
```

For smaller modules

```text
1. AGENT_INSTRUCTIONS.md

2. README.md

3. instruction.md
```

Never skip any document.

---

# Priority

If there is a conflict

```text
instruction-partX.md

        ↓

instruction.md

        ↓

README.md

        ↓

Implementation
```

Instruction files are always the source of truth.

README only explains how to use the module.

---

# Before Writing Code

Understand

- Architecture
- Public API
- Future extensibility
- Coding standards
- Module responsibility

Only then begin implementation.

---

# Project Philosophy

This project is a reusable ESP32 IoT Framework.

It is NOT an application.

Every manager must remain reusable.

Nothing inside a manager should depend on any specific home automation project.

---

# Module Independence

Managers must never directly communicate with each other.

Bad

```text
RelayManager

↓

FirebaseManager
```

Bad

```text
InputManager

↓

RelayManager
```

Good

```text
InputManager

↓

Application

↓

RelayManager
```

The application coordinates modules.

Managers remain independent.

---

# Single Responsibility Principle

Each manager owns exactly one responsibility.

Examples

IoTWiFiManager

- WiFi only

RelayManager

- Relay outputs only

InputManager

- Digital inputs only

StatusLedManager

- RGB Status LED only

FirebaseManager

- Firebase communication only

Never mix responsibilities.

---

# Lifecycle Standard

Every manager follows the same lifecycle.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Follow this convention for every module.

---

# Public API

Public APIs are considered stable.

Never

- Rename public functions
- Change parameter order
- Remove enums
- Remove overloads

unless explicitly instructed.

---

# Constructors

Constructors should never

- connect
- initialize hardware
- allocate resources
- perform network operations

Constructors only initialize internal members.

Hardware initialization belongs in

```cpp
begin();
```

---

# GPIO Ownership

Each hardware manager owns its GPIO.

Examples

RelayManager

Only RelayManager may call

```cpp
digitalWrite();
```

InputManager

Only InputManager may call

```cpp
digitalRead();
```

Application code must never directly manipulate managed GPIO.

---

# Validation

Validate every public function.

Examples

- Invalid channel
- Invalid GPIO
- Invalid state
- Invalid path
- Invalid configuration

Never crash.

Never assume input is valid.

Return the appropriate Result enum.

---

# Error Handling

Never silently ignore failures.

Every public operation should return a Result enum.

Examples

```text
Success

InvalidChannel

InvalidPin

DuplicatePin

Disabled

Busy

Timeout
```

---

# Non-Blocking Design

Never use

```cpp
delay();

while(...)
```

Everything must execute inside

```cpp
loop();
```

using

```cpp
millis();
```

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
- fixed-size arrays
- stack allocation

Suitable for devices running 24/7.

---

# Modern C++

Use

- enum class
- const correctness
- helper functions
- private members
- descriptive names

Readable code is preferred over clever code.

---

# Callback Philosophy

Expose one callback registration whenever possible.

Preferred

```cpp
onEvent(...)
```

Avoid

```cpp
onConnected()

onDisconnected()

onPressed()

onReleased()

onRetry()

...
```

One callback scales better.

---

# Logging

Managers must never use

```cpp
Serial.print();

Serial.println();
```

Future logging belongs to LoggerManager.

---

# Thread Model

Assume Arduino single-threaded execution.

Do not introduce

- FreeRTOS Tasks
- Threads
- Mutexes

unless explicitly required.

---

# Comments

Explain

WHY

not

WHAT

Good

```cpp
// Prevent relay chatter during ESP32 startup.
```

Bad

```cpp
// Turn relay on.
```

---

# Documentation

Every public function should include

- Brief description
- Parameters
- Return value

Prefer Doxygen-style comments.

---

# Performance

Avoid

- unnecessary GPIO writes
- unnecessary GPIO reads
- duplicate operations
- repeated allocations

Keep loop() lightweight.

---

# Future Compatibility

Always design architecture that can grow.

Future features should not require breaking public APIs.

Design for extension.

Do not over-engineer.

---

# Folder Structure

Small modules

```text
Module/
│
├── Module.h
├── Module.cpp
├── README.md
└── instruction.md
```

Large modules

```text
Module/
│
├── Module.h
├── Module.cpp
├── README.md
├── instruction-part1.md
├── instruction-part2.md
├── instruction-part3.md
├── instruction-part4.md
└── instruction-part5.md
```

Maintain this structure.

---

# Implementation Scope

Implement only the requested module.

Do not modify

- other modules
- folder structure
- public APIs

unless explicitly instructed.

---

# If Something Is Unclear

Do not guess.

Do not invent APIs.

Do not redesign the architecture.

Stop and request clarification.

---

# Deliverables

Only implement the requested files.

Do not generate additional modules.

Do not create unnecessary files.

---

# Final Goal

Every manager should feel like it was written by the same developer.

The entire framework should provide

- Consistent APIs
- Consistent coding style
- Consistent folder structure
- Consistent lifecycle
- Consistent documentation
- Consistent architecture

The framework should remain reusable, maintainable and suitable for long-running ESP32 IoT devices.
