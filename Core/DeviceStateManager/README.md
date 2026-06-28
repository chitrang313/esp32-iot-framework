# DeviceStateManager

A reusable runtime state manager for the ESP32 IoT Framework.

DeviceStateManager stores the current runtime state of the device in RAM and provides a single source of truth for all managers.

It does **not** control hardware or save persistent data.

---

# Features

* Store relay states
* Store switch states
* Store WiFi state
* Store Firebase state
* Store device mode
* Runtime state only (RAM)
* Dirty flag tracking
* State version tracking
* Fast state access
* Non-blocking design

---

# Create the manager

```cpp
DeviceStateManager deviceState;
```

---

# Initialize

```cpp
deviceState.begin();
```

---

# Relay State

```cpp
deviceState.setRelayState(
    RelayChannel::Relay1,
    RelayState::On
);

RelayState state =
    deviceState.getRelayState(
        RelayChannel::Relay1
    );
```

---

# Switch State

```cpp
deviceState.setSwitchState(
    SwitchChannel::Switch1,
    SwitchState::Pressed
);

SwitchState state =
    deviceState.getSwitchState(
        SwitchChannel::Switch1
    );
```

---

# WiFi State

```cpp
deviceState.setWiFiState(
    WiFiState::Connected
);

WiFiState state =
    deviceState.getWiFiState();
```

---

# Firebase State

```cpp
deviceState.setFirebaseState(
    FirebaseState::Ready
);

FirebaseState state =
    deviceState.getFirebaseState();
```

---

# Device Mode

```cpp
deviceState.setDeviceMode(
    DeviceMode::Normal
);

DeviceMode mode =
    deviceState.getDeviceMode();
```

---

# Dirty Flags

```cpp
if (deviceState.isRelayDirty())
{
    // Synchronize relay state
}

deviceState.clearRelayDirty();
```

---

# State Version

```cpp
uint32_t version =
    deviceState.getVersion();
```

---

# Runtime Information

```cpp
bool online =
    deviceState.isOnline();

uint32_t uptime =
    deviceState.getUptime();
```

---

# Main Loop

```cpp
void loop()
{
    deviceState.loop();
}
```

---

# Framework Lifecycle

```cpp
deviceState.begin();

deviceState.loop();

deviceState.enable();

deviceState.disable();

deviceState.isEnabled();
```

---

# Notes

* Runtime state is stored in RAM only.
* All runtime state is lost after reboot.
* Persistent data belongs in `PreferencesManager`.
* Each manager updates only its own state.
* Other managers should read state from `DeviceStateManager`.
* Do not duplicate runtime state in the application.
