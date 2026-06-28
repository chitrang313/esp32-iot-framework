# LoggerManager - instruction.md

# LoggerManager

---

# Objective

Develop a reusable, production-quality LoggerManager for the ESP32 IoT Framework.

LoggerManager provides centralized logging for the entire framework.

It receives events from EventBus and outputs log messages to one or more logging targets.

LoggerManager never performs application logic.

---

# Deliverables

Create only these files.

```text
Logging/
└── LoggerManager/
    ├── LoggerManager.h
    ├── LoggerManager.cpp
    ├── README.md
    └── instruction.md
```

Do not create additional public headers.

---

# Purpose

LoggerManager provides

* Framework logging
* Application logging
* Event logging
* Diagnostics

It becomes the single place where all logs are generated.

---

# Responsibilities

LoggerManager is responsible for

* Logging messages
* Log formatting
* Log filtering
* Log levels
* Event logging
* Multiple output targets

LoggerManager is NOT responsible for

* Hardware control
* Runtime state
* Persistent storage
* Synchronization
* Scheduling
* Business logic

---

# Architecture

```text
Framework Modules

↓

EventBus

↓

LoggerManager

↓

Log Output
```

LoggerManager subscribes to EventBus.

Other modules never call LoggerManager directly unless application logging is required.

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

LoggerManager must never block execution.

---

# Public API

Lifecycle

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

Logging

```cpp
debug();

info();

warning();

error();
```

Configuration

```cpp
setLogLevel();

getLogLevel();

enableTimestamp();

disableTimestamp();
```

Statistics

```cpp
getLogCount();

clearLogCount();
```

---

# Log Levels

Use enum class.

```text
Debug

Info

Warning

Error

None
```

Messages below the configured level should be ignored.

---

# Log Format

Recommended format

```text
[12345][INFO] WiFi Connected

[12420][ERROR] Firebase Authentication Failed

[12550][DEBUG] Relay1 -> ON
```

Timestamp should use

```cpp
millis()
```

Architecture should allow timestamps to be disabled.

---

# EventBus Integration

LoggerManager should subscribe to EventBus.

Examples

```text
RelayChanged

↓

Logger
```

```text
WiFiConnected

↓

Logger
```

```text
SyncCompleted

↓

Logger
```

Implementation should use EventBus rather than direct module calls.

---

# Output Targets

Phase 1

* Serial

Future

* SD Card
* SPIFFS
* LittleFS
* Firebase
* MQTT
* USB Serial

Architecture should support additional outputs without changing the public API.

---

# Custom Log Callback

Architecture should support

```cpp
setOutputCallback();
```

Application may provide a custom logging destination.

Implementation optional.

---

# Validation

Validate

* Manager initialized
* Manager enabled
* Valid log level
* Valid message

Never crash.

---

# Error Handling

Use

```text
LoggerResult
```

Examples

```text
Success

Disabled

InvalidLevel

InvalidMessage

Failed
```

Never throw exceptions.

---

# Memory

Avoid

* malloc
* free
* new
* delete

Do not dynamically allocate log buffers.

Use fixed-size buffers where necessary.

---

# Performance

Logging should never block

```cpp
loop();
```

Avoid excessive string formatting.

Messages below the current log level should be discarded immediately.

---

# Dependencies

LoggerManager may communicate only with

* EventBus

LoggerManager must never communicate directly with

* RelayManager
* SwitchManager
* FirebaseManager
* SyncManager
* PreferencesManager
* DeviceStateManager
* Scheduler

---

# Documentation

Every public function should include

* Brief description
* Parameters
* Return value

Use Doxygen-style comments.

---

# Coding Standards

Use

* enum class
* const correctness
* helper functions
* descriptive names
* private members
* readable code

Prefer clarity over clever implementations.

---

# Future Compatibility

Architecture should support

* Multiple output targets
* File logging
* Cloud logging
* Log rotation
* Log filtering
* Colored terminal output
* Remote debugging
* Persistent log storage

without changing the public API.

Implementation is NOT required.

---

# Final Goal

LoggerManager becomes the centralized logging system for the ESP32 IoT Framework.

Framework modules publish events.

LoggerManager receives those events through EventBus.

LoggerManager formats and outputs log messages according to the configured log level.

It remains lightweight, reusable, non-blocking, and independent of application logic.
