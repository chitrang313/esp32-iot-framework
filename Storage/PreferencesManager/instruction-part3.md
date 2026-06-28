# PreferencesManager - instruction-part3.md

# Part 3 - Reliability, Flash Protection & Data Integrity

---

# Objective

PreferencesManager must protect the ESP32 flash memory while ensuring reliable persistent storage.

The manager should maximize flash lifetime, prevent unnecessary writes, and recover safely from common storage errors.

The application should never manage flash reliability.

---

# Design Philosophy

The application requests

```cpp
preferences.save(...);
```

PreferencesManager decides

* Whether a write is necessary
* When to write
* Whether validation is required
* Whether the write succeeded

The application should never interact with the storage engine directly.

---

# Flash Wear Protection

Flash memory has a limited write lifetime.

PreferencesManager must minimize unnecessary writes.

Never write identical values repeatedly.

Example

```cpp
preferences.save(
    PreferenceKey::Relay1State,
    true
);

preferences.save(
    PreferenceKey::Relay1State,
    true
);
```

The second call should detect that the value has not changed.

Do not perform another flash write.

Return

```text
PreferencesResult::Success
```

---

# Duplicate Write Detection

Before writing

↓

Read existing value

↓

Compare values

↓

If identical

↓

Skip write

↓

Return Success

This behavior should be automatic.

---

# Atomic Updates

Every save operation should behave atomically.

Never leave partially written data.

If a write fails

↓

Keep the previous value intact.

Never corrupt stored data.

---

# Data Integrity

After every successful write

↓

The stored value should be immediately readable.

Example

```cpp
preferences.save(
    PreferenceKey::DeviceName,
    "Living Room"
);

preferences.load(
    PreferenceKey::DeviceName,
    deviceName
);
```

The loaded value must match the saved value.

---

# Storage Errors

Handle common storage failures.

Examples

* Flash write failure
* Flash read failure
* Invalid key
* Invalid value
* Storage full
* Corrupted storage

Return

```text
PreferencesResult
```

Never crash.

---

# Recovery Strategy

If storage initialization fails

↓

Move manager into

```text
PreferencesState::Error
```

Do not continue writing.

Application may observe the state.

---

# Versioning

Architecture should support future storage versioning.

Example

```text
Storage Version 1

↓

Storage Version 2
```

Future firmware may migrate old settings automatically.

Do not implement migration in Phase 1.

---

# Migration

Future architecture should support

```text
Old Key

↓

New Key
```

Example

```text
wifi.name

↓

wifi.ssid
```

Migration should occur internally.

Application code should not perform migrations.

---

# Backup Strategy

Future architecture should support

* Configuration backup
* Configuration restore

Do not implement.

---

# Factory Reset

Architecture should support

```cpp
preferences.factoryReset();
```

Behavior

* Remove all stored values
* Restore default state
* Reinitialize storage

Do not implement in Phase 1.

---

# Key Validation

Keys should follow a consistent format.

Recommended

```text
wifi.ssid

wifi.password

firebase.apiKey

relay.1.state

device.name
```

Reject

```text
""

" "

".."

"///"

```

Return

```text
PreferencesResult::InvalidKey
```

---

# Value Validation

Reject invalid values before writing.

Examples

* Null pointers
* Unsupported data types
* Corrupted data

Never write invalid data.

---

# Storage Capacity

Architecture should expose

```cpp
preferences.getFreeSpace();

preferences.getUsedSpace();
```

Implementation is optional in Phase 1.

Future support is recommended.

---

# Statistics

Future architecture should support

* Total Writes
* Skipped Writes
* Read Count
* Write Count
* Remove Count
* Error Count

Do not implement.

---

# Diagnostics

Future architecture should support

```cpp
preferences.getLastError();
```

and

```cpp
preferences.getStatistics();
```

Implementation is not required.

---

# Performance

Storage operations should complete quickly.

Avoid

* Repeated flash writes
* Duplicate reads
* Large temporary buffers

Never block

```cpp
loop();
```

---

# Memory

Avoid

* Dynamic allocation
* Heap fragmentation

Prefer fixed-size buffers.

Suitable for continuous operation.

---

# Future Compatibility

Architecture should support

* Encryption
* Compression
* Binary blobs
* JSON
* Structures
* SD Card backend
* LittleFS backend
* External EEPROM backend

without changing the public API.

---

# Final Goal

Application code should simply call

```cpp
preferences.save(...);

preferences.load(...);
```

PreferencesManager automatically provides

* Flash wear protection
* Duplicate write detection
* Data integrity
* Validation
* Error recovery
* Reliable persistent storage

The application should never manage flash memory directly.
