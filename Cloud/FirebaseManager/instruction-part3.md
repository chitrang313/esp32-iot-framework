# FirebaseManager - instruction-part3.md

# Part 3 - Read, Write, Streaming & Event System

---

# Objective

This part defines how FirebaseManager exchanges data with Firebase Realtime Database.

FirebaseManager must provide a clean, type-safe API for reading, writing, and subscribing to database changes.

The application must never call Firebase client library functions directly.

---

# Design Philosophy

Provide a minimal public API.

Avoid multiple function names such as

```cpp
setBool()

setInt()

setFloat()

setString()
```

Instead provide overloaded functions.

```cpp
set(path, value);

get(path, value);
```

The compiler selects the correct overload.

---

# Supported Data Types

The public API should support

- bool
- int
- float
- double
- const char\*
- JsonDocument

Future versions should allow additional data types without changing the API.

---

# Read Operations

Examples

```cpp
bool light;

firebase.get(
    FirebasePath::KitchenLight,
    light
);
```

```cpp
int counter;

firebase.get(
    FirebasePath::Counter,
    counter
);
```

```cpp
float temperature;

firebase.get(
    FirebasePath::Temperature,
    temperature
);
```

```cpp
JsonDocument config;

firebase.get(
    FirebasePath::Configuration,
    config
);
```

Every read operation returns

```text
FirebaseResult
```

---

# Write Operations

Examples

```cpp
firebase.set(
    FirebasePath::KitchenLight,
    true
);
```

```cpp
firebase.set(
    FirebasePath::Counter,
    15
);
```

```cpp
firebase.set(
    FirebasePath::Temperature,
    24.75f
);
```

```cpp
firebase.set(
    FirebasePath::Configuration,
    jsonDocument
);
```

Every write operation returns

```text
FirebaseResult
```

---

# Validation

Before every read/write operation verify

- Module enabled
- Configured
- WiFi Connected
- Firebase Connected
- Authenticated
- Ready
- Valid path
- Valid value

If validation fails

Return the appropriate

```text
FirebaseResult
```

Never crash.

Never block.

Never silently fail.

---

# Database Paths

Database paths belong to the application.

Never hardcode database paths inside FirebaseManager.

Recommended

```cpp
namespace FirebasePath
{
    constexpr const char* KitchenLight =
        "/home/kitchen/light";

    constexpr const char* Fan =
        "/home/bedroom/fan";

    constexpr const char* Temperature =
        "/home/livingroom/temperature";
}
```

Application should always use constants.

Avoid string literals throughout the project.

---

# Streaming

Support realtime subscriptions.

Example

```cpp
firebase.subscribe(
    FirebasePath::KitchenLight
);
```

Multiple subscriptions should be supported.

Provide

```cpp
unsubscribe(path);

unsubscribeAll();
```

Application code should never manage Firebase stream objects.

---

# Stream Updates

When Firebase detects a value change

↓

FirebaseManager validates the update

↓

Updates internal state if required

↓

Publishes an event

↓

Application decides what to do

FirebaseManager must never control relays directly.

---

# Event System

Expose only one callback registration.

```cpp
firebase.onEvent(
    onFirebaseEvent
);
```

Avoid multiple callback registration APIs.

---

# Callback Philosophy

Every Firebase activity generates an event.

Examples

- Connected
- Disconnected
- ReadCompleted
- WriteCompleted
- StreamUpdated
- QueueFlushed
- RetryStarted
- RetryCompleted
- Error

The application receives one callback.

---

# Callback Parameters

The callback should provide

- FirebaseEvent
- FirebaseResult
- FirebaseDataType
- Database Path
- Optional Context Pointer

Example

```cpp
void onFirebaseEvent(
    FirebaseEvent event,
    FirebaseResult result,
    FirebaseDataType type,
    const char* path,
    void* context
)
{
}
```

The implementation may extend the callback later without breaking compatibility.

---

# Event Ownership

FirebaseManager only publishes events.

The application decides how to respond.

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

Never

```text
FirebaseManager

↓

RelayManager
```

---

# Read Cache

Do not implement.

Architecture should allow adding

- Read Cache
- Cache Expiration
- Cache Synchronization

later without changing the public API.

---

# Write Cache

Do not implement.

Architecture should support

- Deferred Writes
- Cached Writes
- Offline Writes

later.

---

# Batch Operations

Do not implement.

The architecture should support

```cpp
firebase.beginBatch();

firebase.set(...);

firebase.set(...);

firebase.commitBatch();
```

in future versions.

---

# Transactions

Do not implement.

Architecture should allow future support for

- Atomic Updates
- Transactions
- Compare-and-Swap

without changing the API.

---

# Performance

Avoid unnecessary reads.

Avoid unnecessary writes.

Avoid duplicate subscriptions.

Avoid unnecessary memory allocations.

Never block loop().

---

# Memory

Avoid

- String
- new
- delete
- malloc
- free

Prefer fixed-size buffers where practical.

Suitable for long-running embedded systems.

---

# Future Compatibility

Architecture should support

- Firestore
- Cloud Storage
- Remote Config
- Cloud Functions
- Multiple Firebase Projects

without changing the public API.

---

# Final Goal

Application code should simply write

```cpp
firebase.get(...);

firebase.set(...);

firebase.subscribe(...);

firebase.onEvent(...);
```

FirebaseManager performs

- Validation
- Type Selection
- Firebase Communication
- Stream Processing
- Event Publishing

The application only consumes events and decides how other managers should respond.
