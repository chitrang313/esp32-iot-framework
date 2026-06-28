# PreferencesManager - instruction-part2.md

# Part 2 - Read / Write API & Storage Operations

---

# Objective

This part defines how the application stores and retrieves persistent values.

PreferencesManager should provide a clean, type-safe key-value storage API.

The application should never interact with the ESP32 Preferences library directly.

---

# Storage Philosophy

Every stored value is identified by a unique key.

Example

```text
wifi.ssid

wifi.password

firebase.apiKey

firebase.databaseUrl

relay.1.state

relay.2.state

device.name

device.version

led.brightness
```

The application owns the keys.

PreferencesManager owns the storage.

---

# Public API

Expose only the following public functions.

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
getState();

isReady();
```

---

# Save API

Do NOT expose

```cpp
putBool();

putInt();

putFloat();

putString();
```

Instead provide overloaded APIs.

Examples

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
    "led.brightness",
    80
);

preferences.save(
    "temperature.offset",
    2.5f
);
```

Every overload returns

```text
PreferencesResult
```

---

# Load API

Do NOT expose

```cpp
getBool();

getInt();

getFloat();

getString();
```

Instead

```cpp
preferences.load(
    "wifi.ssid",
    ssid
);

preferences.load(
    "relay.1.state",
    relayState
);

preferences.load(
    "led.brightness",
    brightness
);
```

Every overload returns

```text
PreferencesResult
```

---

# Exists

Application may verify whether a key exists.

Example

```cpp
preferences.exists(
    "wifi.ssid"
);
```

Returns

```cpp
bool
```

---

# Remove

Application may remove a single key.

Example

```cpp
preferences.remove(
    "wifi.password"
);
```

Returns

```text
PreferencesResult
```

---

# Clear

Erase every stored value.

Example

```cpp
preferences.clear();
```

Returns

```text
PreferencesResult
```

Use with caution.

---

# Default Values

Architecture should support

```cpp
preferences.load(
    "relay.1.state",
    relayState,
    false
);
```

Meaning

If the key does not exist

↓

Return

```cpp
false
```

without requiring the application to check

```cpp
exists()
```

Implementation is optional in Phase 1.

The architecture should support it.

---

# Duplicate Writes

Before writing

↓

Compare the existing value.

If unchanged

↓

Do not write to flash.

Return

```text
PreferencesResult::Success
```

This reduces flash wear.

---

# Validation

Before every operation verify

* Manager initialized
* Manager enabled
* Valid key
* Supported type
* Valid destination pointer

Return

```text
PreferencesResult
```

Never crash.

---

# Key Naming Rules

Recommended

```text
wifi.ssid

wifi.password

firebase.apiKey

relay.1.state

relay.2.state

switch.1.mode

device.name

device.version

system.language
```

Avoid

```text
SSID

password

abc

value1
```

Keys should be descriptive.

---

# Namespace Handling

The application should never call

```cpp
begin(namespace);
```

PreferencesManager owns namespace management internally.

Future storage implementations should not require application changes.

---

# Return Values

Every public API should return

```text
PreferencesResult
```

Never silently fail.

Never throw exceptions.

---

# State Management

Maintain an internal state.

Example

```text
Uninitialized

↓

Ready

↓

Disabled

↓

Error
```

Application may query

```cpp
preferences.getState();
```

Application must never modify the state.

---

# Future Compatibility

Architecture should support

* Arrays
* Binary blobs
* JSON
* Structures
* Versioned records

without changing the public API.

---

# Performance

Avoid unnecessary flash writes.

Avoid duplicate writes.

Keep operations lightweight.

Never block execution.

---

# Memory

Avoid

* Heap fragmentation
* Large temporary buffers

Prefer stack allocation and fixed-size buffers.

---

# Final Goal

The application should simply write

```cpp
preferences.save(...);

preferences.load(...);

preferences.exists(...);

preferences.remove(...);

preferences.clear();
```

PreferencesManager handles

* Type selection
* Validation
* Flash storage
* Duplicate protection
* Error handling

The application should never interact with the ESP32 Preferences library directly.
