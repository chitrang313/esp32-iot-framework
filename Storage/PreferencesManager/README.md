# PreferencesManager

A reusable ESP32 manager for persistent storage.

PreferencesManager provides a simple, type-safe API for saving and loading settings using the ESP32's non-volatile storage (NVS). It completely hides the ESP32 `Preferences` library from the application.

---

# Features

* Save persistent values
* Load persistent values
* Check if a key exists
* Remove stored values
* Clear all stored values
* Flash wear protection
* Duplicate write protection
* Type-safe API
* Non-blocking design
* Framework-friendly architecture

---

# Supported Data Types

* bool
* int
* uint32_t
* float
* double
* const char*

---

# Basic Usage

## Create the manager

```cpp
PreferencesManager preferences;
```

---

## Initialize

```cpp
preferences.begin();
```

---

## Save values

```cpp
preferences.save(
    PreferenceKey::WiFiSSID,
    ssid
);

preferences.save(
    PreferenceKey::Relay1State,
    true
);

preferences.save(
    PreferenceKey::LedBrightness,
    80
);
```

---

## Load values

```cpp
char ssid[64];

preferences.load(
    PreferenceKey::WiFiSSID,
    ssid
);

bool relayState;

preferences.load(
    PreferenceKey::Relay1State,
    relayState
);
```

---

## Check if a key exists

```cpp
if (preferences.exists(PreferenceKey::WiFiSSID))
{
    // Key exists
}
```

---

## Remove a key

```cpp
preferences.remove(
    PreferenceKey::WiFiSSID
);
```

---

## Clear all stored data

```cpp
preferences.clear();
```

---

## Check manager state

```cpp
if (preferences.isReady())
{
    // Storage is ready
}
```

---

## Main Loop

```cpp
void loop()
{
    preferences.loop();
}
```

---

# Example Keys

```cpp
namespace PreferenceKey
{
    constexpr const char* WiFiSSID          = "wifi.ssid";
    constexpr const char* WiFiPassword      = "wifi.password";

    constexpr const char* FirebaseApiKey    = "firebase.apiKey";
    constexpr const char* FirebaseDatabase  = "firebase.databaseUrl";

    constexpr const char* Relay1State       = "relay.1.state";

    constexpr const char* DeviceName        = "device.name";

    constexpr const char* LedBrightness     = "led.brightness";
}
```

---

# Framework Lifecycle

```cpp
preferences.begin();

preferences.loop();

preferences.enable();

preferences.disable();

preferences.isEnabled();
```

---

# Notes

* Do not use the ESP32 `Preferences` library directly.
* Store all persistent values through `PreferencesManager`.
* Use descriptive keys for better maintainability.
* Define all keys in a central `PreferenceKey` namespace.
* Call `loop()` from the main application loop for framework consistency.
