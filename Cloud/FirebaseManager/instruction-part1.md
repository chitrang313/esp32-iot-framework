# FirebaseManager - instruction-part1.md

# Part 1 - Foundation & Architecture

---

# Objective

Develop a reusable, production-quality Firebase Realtime Database Manager for ESP32.

FirebaseManager is responsible for all communication between the ESP32 and Firebase Realtime Database.

It must completely abstract the Firebase client library from the application.

Application code must never directly use:

- FirebaseData
- FirebaseAuth
- FirebaseConfig
- FirebaseJson
- FirebaseJsonArray
- Firebase.RTDB
- Firebase.ready()
- Firebase.signUp()
- Firebase.begin()

Everything must be accessed through FirebaseManager.

The goal is to make changing the underlying Firebase library possible without modifying application code.

---

# Deliverables

Create only these files.

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

Do not create additional public headers.

Internal helper classes may be implemented privately inside `FirebaseManager.cpp`.

---

# Purpose

FirebaseManager becomes the single owner of every Firebase operation.

Examples

- Authentication
- Token Management
- Read Operations
- Write Operations
- Streaming
- Offline Queue
- Retry Engine
- Synchronization

The application should never communicate with Firebase directly.

---

# Design Principles

The implementation must follow

- Single Responsibility Principle
- Open / Closed Principle
- Encapsulation
- Modern C++
- Clean Architecture

FirebaseManager must only manage Firebase communication.

Nothing else.

---

# Responsibilities

FirebaseManager is responsible for

- Firebase configuration
- Authentication
- Token refresh
- Connection management
- Read operations
- Write operations
- Streaming
- Automatic reconnection
- Offline queue
- Retry engine
- Callback events
- Connection state

FirebaseManager is **NOT** responsible for

- Relay control
- Input handling
- LED control
- WiFi management
- MQTT
- OTA
- Preferences
- Scheduling
- Automation logic

Those belong to their own managers.

---

# Architecture

Every framework module must remain independent.

Example

```text
Application
      │
      ▼
FirebaseManager
      │
      ▼
Firebase Client Library
      │
      ▼
Firebase Realtime Database
```

Never

```text
FirebaseManager

↓

RelayManager
```

Never

```text
FirebaseManager

↓

StatusLedManager
```

Never

```text
FirebaseManager

↓

InputManager
```

The application coordinates modules.

---

# Internal Architecture

Internally FirebaseManager should separate responsibilities.

Example

```text
FirebaseManager
│
├── Configuration
├── Authentication
├── Connection
├── Read
├── Write
├── Stream
├── Queue
├── Retry
└── Event Dispatcher
```

These are **internal implementation details**.

Only one public class should exist.

---

# Module Lifecycle

Every framework manager follows the same lifecycle.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

FirebaseManager must follow this standard.

---

# Object Creation

The constructor must not require configuration.

Example

```cpp
FirebaseManager firebase;
```

The constructor should only initialize internal variables.

No network operations.

No authentication.

No Firebase communication.

---

# Configuration

The application configures Firebase before calling `begin()`.

Example

```cpp
firebase.configure(
    API_KEY,
    DATABASE_URL,
    USER_EMAIL,
    USER_PASSWORD
);
```

Configuration only stores values.

It must not establish a connection.

---

# Configuration Rules

- Configuration is allowed only before `begin()`
- Configuration must be validated
- Empty values must be rejected
- Invalid URLs must be rejected
- Duplicate configuration must be rejected
- Calling configure() after begin() must return an error

---

# Application Example

```cpp
FirebaseManager firebase;

void setup()
{
    firebase.configure(
        API_KEY,
        DATABASE_URL,
        USER_EMAIL,
        USER_PASSWORD
    );

    firebase.begin();
}

void loop()
{
    firebase.loop();
}
```

---

# Firebase Paths

FirebaseManager must remain generic.

Database paths belong to the application.

Example

```cpp
namespace FirebasePath
{
    constexpr const char* KitchenLight =
        "/home/kitchen/light";

    constexpr const char* Fan =
        "/home/bedroom/fan";
}
```

Application code becomes

```cpp
firebase.set(
    FirebasePath::KitchenLight,
    true
);
```

instead of

```cpp
firebase.set(
    "/home/kitchen/light",
    true
);
```

Avoid hardcoded path strings throughout the application.

---

# Public API Philosophy

The API should be simple.

Instead of many function names

```cpp
setBool()

setInt()

setFloat()

setString()
```

Use overloaded functions.

```cpp
set(path, value);
```

Likewise

```cpp
get(path, value);
```

The compiler selects the correct overload based on the value type.

This keeps the API clean and scalable.

---

# Supported Data Types

The public API should support

- bool
- int
- float
- double
- const char\*
- JSON

Future versions may support additional types without changing the API.

---

# Final Goal (Part 1)

The application should never know anything about the Firebase client library.

Instead of writing

```cpp
Firebase.RTDB.setBool(...)
```

the application simply writes

```cpp
firebase.set(...);

firebase.get(...);

firebase.subscribe(...);
```

FirebaseManager becomes the single source of truth for every Firebase operation inside the framework.

# Part 1B - Public API & Enumerations

---

# Public API

FirebaseManager must expose only the following public functions.

## Lifecycle

```cpp
configure(...);

begin();

loop();

enable();

disable();

isEnabled();
```

---

## Connection

```cpp
connect();

disconnect();

reconnect();
```

These functions should not block.

The implementation should use an internal state machine.

---

## Information

```cpp
isConfigured();

isConnected();

isAuthenticated();

isReady();

getState();
```

These functions return the current internal state.

Application code should never inspect Firebase library objects.

---

# Read API

Do NOT expose

```cpp
getBool();

getInt();

getFloat();

getDouble();

getString();

getJson();
```

Instead use overloaded functions.

Examples

```cpp
bool value;
firebase.get(path, value);
```

```cpp
int counter;
firebase.get(path, counter);
```

```cpp
float temperature;
firebase.get(path, temperature);
```

```cpp
double voltage;
firebase.get(path, voltage);
```

```cpp
const char* deviceName;
firebase.get(path, deviceName);
```

```cpp
JsonDocument config;
firebase.get(path, config);
```

Every overload returns

```cpp
FirebaseResult
```

---

# Write API

Do NOT expose

```cpp
setBool();

setInt();

setFloat();

setDouble();

setString();

setJson();
```

Instead use overloaded functions.

Examples

```cpp
firebase.set(path, true);

firebase.set(path, 25);

firebase.set(path, 24.6f);

firebase.set(path, 220.45);

firebase.set(path, "Kitchen");

firebase.set(path, jsonDocument);
```

Every overload returns

```cpp
FirebaseResult
```

---

# Streaming API

Provide APIs similar to

```cpp
subscribe(path);

unsubscribe(path);

unsubscribeAll();
```

The application may subscribe to multiple paths.

FirebaseManager maintains the internal subscription list.

---

# Callback System

FirebaseManager should expose only ONE callback registration.

Example

```cpp
onEvent(callback);
```

Do NOT expose

```cpp
onConnected();

onDisconnected();

onWriteSuccess();

onWriteFailed();

onReadSuccess();
```

A single callback keeps the API scalable.

---

# Callback Signature

The callback should provide enough information for the application to decide what to do.

Conceptually

```cpp
FirebaseEvent

FirebaseResult

const char* path

FirebaseDataType

void* context
```

The implementation may extend this later without breaking compatibility.

---

# Event Philosophy

FirebaseManager only publishes events.

Application code decides how to respond.

Example

```text
Firebase Updated

↓

FirebaseManager

↓

Application

↓

RelayManager
```

NOT

```text
FirebaseManager

↓

RelayManager
```

---

# Enumerations

Use enum class wherever possible.

---

## FirebaseState

```text
NotConfigured

Configured

Connecting

Authenticating

Connected

Ready

Disconnected

Disabled

Error
```

This represents the current internal state.

Only one state may be active at a time.

---

## FirebaseResult

```text
Success

Queued

Busy

InvalidPath

InvalidValue

InvalidType

NotConfigured

NotConnected

AuthenticationFailed

PermissionDenied

Timeout

Disabled

ConfigurationClosed

AlreadyConfigured

AlreadyConnected

AlreadyDisconnected

SubscriptionExists

SubscriptionNotFound

UnknownError
```

Every public API returns one of these values.

Never silently fail.

---

## FirebaseEvent

```text
Connected

Disconnected

Authenticated

Ready

ReadCompleted

WriteCompleted

StreamUpdated

SubscriptionAdded

SubscriptionRemoved

QueueFlushed

RetryStarted

RetryCompleted

Error
```

Future events may be added without changing the callback API.

---

## FirebaseDataType

```text
Boolean

Integer

Float

Double

String

Json

Array

Unknown
```

Used by callbacks.

---

# State Machine

FirebaseManager should internally manage state transitions.

Example

```text
NotConfigured

↓

Configured

↓

Connecting

↓

Authenticating

↓

Connected

↓

Ready
```

If WiFi disconnects

```text
Ready

↓

Disconnected

↓

Connecting

↓

Authenticating

↓

Ready
```

Application code should never perform this logic.

---

# State Ownership

Only FirebaseManager may modify

```text
FirebaseState
```

Application code is read-only.

---

# Public API Rules

Public APIs must

- validate parameters
- never crash
- never block
- never throw exceptions
- return FirebaseResult
- remain backward compatible

---

# Future Compatibility

The public API should remain stable even if

- Firebase Arduino Client changes
- another Firebase library is adopted
- Firestore support is added
- Cloud Storage is added

The application should never notice internal implementation changes.

---

# Final Goal (Part 1B)

The public API should be small, consistent and type-safe.

Application code should look like

```cpp
firebase.configure(...);

firebase.begin();

firebase.set(...);

firebase.get(...);

firebase.subscribe(...);

firebase.onEvent(...);
```

without ever accessing the Firebase client library directly.
