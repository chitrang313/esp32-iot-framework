# SyncManager

A reusable synchronization manager for the ESP32 IoT Framework.

SyncManager keeps the runtime state synchronized between the device and cloud services.

It does **not** own runtime state, control hardware, or communicate directly with application logic.

---

# Features

* Synchronize runtime state
* Local → Cloud synchronization
* Cloud → Local synchronization
* Dirty flag processing
* Synchronization queue
* Automatic retry
* Conflict prevention
* Synchronization statistics
* Non-blocking design
* Framework-friendly architecture

---

# Create the manager

```cpp
SyncManager syncManager;
```

---

# Initialize

```cpp
syncManager.begin();
```

---

# Synchronize Changes

```cpp
syncManager.sync();
```

Synchronizes all pending runtime changes.

---

# Full Synchronization

```cpp
syncManager.syncAll();
```

Synchronizes the complete runtime state.

Useful after

* Device boot
* WiFi reconnect
* Firebase reconnect

---

# Retry Synchronization

```cpp
syncManager.retrySync();
```

Retries failed synchronization requests.

---

# Cancel Synchronization

```cpp
syncManager.cancelSync();
```

Stops the current synchronization safely.

---

# Synchronization Status

```cpp
SyncState state =
    syncManager.getState();

bool busy =
    syncManager.isBusy();

bool idle =
    syncManager.isIdle();
```

---

# Statistics

```cpp
uint32_t pending =
    syncManager.getPendingCount();

uint32_t success =
    syncManager.getSuccessfulSyncCount();

uint32_t failed =
    syncManager.getFailedSyncCount();
```

---

# Main Loop

```cpp
void loop()
{
    syncManager.loop();
}
```

---

# Framework Lifecycle

```cpp
syncManager.begin();

syncManager.loop();

syncManager.enable();

syncManager.disable();

syncManager.isEnabled();
```

---

# Synchronization Flow

```text
RelayManager
        │
        ▼
DeviceStateManager
        │
        ▼
SyncManager
        │
        ▼
FirebaseManager
        │
        ▼
Firebase
```

---

# Notes

* SyncManager does not own runtime state.
* Runtime state belongs to `DeviceStateManager`.
* Cloud communication belongs to `FirebaseManager`.
* Synchronization is non-blocking.
* Dirty flags are cleared only after successful synchronization.
* Failed synchronization is retried automatically.
* SyncManager coordinates synchronization only.
