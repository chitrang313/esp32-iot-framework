# PreferencesManager - instruction-part1.md

# Part 1 - Foundation & Architecture

---

# Objective

Develop a reusable, production-quality persistent storage manager for ESP32.

PreferencesManager is responsible for storing and retrieving persistent configuration and application data.

The manager must completely hide the ESP32 Preferences (NVS) library from the application.

Application code must never directly use

* Preferences
* begin()
* end()
* putXXX()
* getXXX()
* remove()
* clear()

Everything must go through PreferencesManager.

The goal is to make changing the storage backend possible without modifying application code.

---

# Deliverables

Create only these files.

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

Do not create additional public headers.

Private helper classes may exist inside `PreferencesManager.cpp`.

---

# Purpose

PreferencesManager becomes the single owner of all persistent storage.

Examples

* WiFi Credentials
* Firebase Configuration
* Device Name
* Relay Restore State
* LED Brightness
* Calibration Values
* User Settings
* Feature Flags
* Firmware Version
* Device Configuration

The application should never access ESP32 NVS directly.

---

# Design Principles

Follow

* Single Responsibility Principle
* Open / Closed Principle
* Modern C++
* Clean Architecture
* Encapsulation

PreferencesManager owns persistent storage only.

Nothing else.

---

# Responsibilities

PreferencesManager is responsible for

* Saving values
* Loading values
* Removing values
* Clearing values
* Checking key existence
* Storage validation
* Storage versioning
* Storage migration (future)

PreferencesManager is NOT responsible for

* WiFi
* Firebase
* Relay Control
* Input Handling
* Scheduling
* Logging
* Automation

---

# Storage Architecture

Application

↓

PreferencesManager

↓

ESP32 Preferences (NVS)

↓

Flash Memory

The application must never access NVS directly.

---

# Lifecycle

Every framework manager follows

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

PreferencesManager must implement the same lifecycle.

Although storage operations are mostly immediate, `loop()` must exist for future background features.

---

# Object Creation

The constructor must not access flash.

Example

```cpp
PreferencesManager preferences;
```

The constructor only initializes internal members.

No storage access.

No namespace creation.

No flash operations.

---

# Public API Philosophy

Provide a small, consistent API.

Do NOT expose

```cpp
putString()

putInt()

putBool()

getString()

getInt()

getBool()
```

Instead use overloaded functions.

Example

```cpp
preferences.save(
    "wifi.ssid",
    ssid
);

preferences.save(
    "relay.1.state",
    true
);

preferences.save(
    "brightness",
    80
);
```

Reading

```cpp
char ssid[32];

preferences.load(
    "wifi.ssid",
    ssid
);

bool relayState;

preferences.load(
    "relay.1.state",
    relayState
);
```

The compiler selects the correct overload.

---

# Flat Key Design

The application should never manage namespaces.

Instead use descriptive keys.

Examples

```text
wifi.ssid

wifi.password

firebase.url

firebase.apiKey

relay.1.state

relay.2.state

device.name

device.location

led.brightness
```

PreferencesManager internally manages namespaces if required.

This keeps the public API storage-backend independent.

---

# Supported Data Types

The public API should support

* bool
* int
* unsigned int
* long
* unsigned long
* float
* double
* const char*
* char[]
* Binary Data (future)

Future versions should allow additional types without changing the API.

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

Storage

```cpp
save(...);

load(...);

exists(...);

remove(...);

clear();
```

Information

```cpp
isReady();

getState();

getStorageVersion();
```

---

# Enumerations

Use enum class.

## PreferencesState

```text
NotInitialized

Ready

Disabled

Busy

Error
```

---

## PreferencesResult

```text
Success

KeyNotFound

InvalidKey

InvalidValue

StorageFull

StorageError

Disabled

Busy

UnknownError
```

Every public function returns a PreferencesResult.

Never silently fail.

---

# Validation

Validate every public function.

Examples

* Empty key
* Invalid key
* Null pointer
* Disabled manager
* Storage unavailable

Never assume parameters are valid.

---

# Memory Rules

Avoid

* String
* new
* delete
* malloc
* free

Prefer

* constexpr
* fixed-size buffers
* stack allocation

Suitable for devices running continuously.

---

# Logging

PreferencesManager must never use

```cpp
Serial.print();

Serial.println();
```

Future logging belongs to LoggerManager.

---

# Non-Blocking

Never use

```cpp
delay();

while(...)
```

All operations must complete quickly.

Future background work belongs inside `loop()`.

---

# Future Storage Backends

The architecture should allow replacing ESP32 Preferences with

* EEPROM
* LittleFS
* SPIFFS
* SD Card
* External Flash

without changing the public API.

---

# Final Goal

Application code should never know which storage backend is used.

Instead of

```cpp
Preferences preferences;

preferences.putString(...);
```

the application simply writes

```cpp
preferences.save(...);

preferences.load(...);

preferences.exists(...);

preferences.remove(...);
```

PreferencesManager becomes the single source of truth for persistent storage across the entire framework.
