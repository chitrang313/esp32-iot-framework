# Task: Build a Production-Quality ESP32 IoTWiFiManager Module

## Objective

Create a reusable, production-quality WiFi manager for ESP32.

This module will become the networking foundation for all future ESP32 IoT projects.

The implementation must follow clean architecture principles, modern C++ practices, and event-driven design.

The goal is reliability, maintainability, and reusability—not simply connecting to WiFi.

---

# Deliverables

Only create these two files.

```text
Network/
├── IoTWiFiManager.h
└── IoTWiFiManager.cpp
```

Do NOT generate example sketches.

Do NOT generate Config files.

Do NOT generate LED code.

Do NOT generate Firebase code.

Focus only on the WiFi module.

---

# Design Principles

The module must:

* Be completely self-contained.
* Be reusable across multiple ESP32 projects.
* Never depend on Firebase, MQTT, LEDs, Relays, OTA, or Sensors.
* Never contain application logic.
* Expose a clean public API.
* Hide all implementation details.
* Be suitable for long-running IoT devices.

---

# ESP32 Target

* Arduino Core 3.x
* ESP32-WROOM / ESP32-S3 compatible
* Use only the built-in WiFi library

No third-party networking libraries.

---

# Constructor

The constructor should accept:

```cpp
const char* ssid
const char* password
```

Credentials must NOT be hardcoded.

---

# Public API

Implement a clean API similar to:

```cpp
class IoTWiFiManager
{
public:

    IoTWiFiManager(const char* ssid,
                   const char* password);

    void begin();

    void loop();

    bool isConnected() const;

    bool isConnecting() const;

    WiFiState getState() const;

    String getIPAddress() const;

    String getSSID() const;

    int32_t getRSSI() const;

    uint32_t getReconnectCount() const;

    uint32_t getConnectedDuration() const;

    int getLastDisconnectReason() const;

    void reconnect();

    void disconnect();

};
```

The API should remain stable for future projects.

---

# Internal State Machine

Do NOT use multiple boolean flags.

Implement a proper state machine.

```cpp
enum class WiFiState
{
    Idle,
    Connecting,
    Connected,
    Reconnecting,
    Disconnected
};
```

All transitions must be explicit.

---

# Event Driven Architecture

Register ESP32 WiFi events.

Use:

```cpp
WiFi.onEvent(...)
```

The event callback must ONLY:

* Update internal state
* Save timestamps
* Save disconnect reason
* Update reconnect flags

Never reconnect inside the callback.

Never call WiFi.begin() inside the callback.

---

# Connection Logic

The module must own all WiFi connection logic.

Private helper methods are recommended:

```cpp
connect()

disconnectInternal()

scheduleReconnect()

resetReconnectState()

updateState()
```

Avoid duplicated code.

---

# Reconnection Strategy

Implement manual reconnection.

Requirements:

* Never block.
* Never use while() waiting for WiFi.
* Never spam WiFi.begin().
* Retry from loop().
* Retry only when necessary.

---

# Exponential Backoff

Reconnect delay should increase automatically.

Example:

Attempt 1

2 seconds

Attempt 2

4 seconds

Attempt 3

8 seconds

Attempt 4

16 seconds

Attempt 5

30 seconds

Attempt 6+

60 seconds maximum

Reset the retry counter immediately after a successful connection.

---

# Connection Timeout

If connecting exceeds a configurable timeout,

transition back to Reconnecting.

Never remain indefinitely in Connecting.

---

# Disconnect Handling

Capture and expose the ESP32 disconnect reason.

Store it internally.

Expose:

```cpp
getLastDisconnectReason()
```

This is critical for diagnostics.

---

# Logging

The module should log useful information to Serial.

Examples:

```
[WiFi] Connecting...

[WiFi] Connected

[WiFi] IP: 192.168.1.100

[WiFi] RSSI: -48 dBm

[WiFi] Disconnected

[WiFi] Reason: 201

[WiFi] Retry in 8 seconds
```

Logging should be easy to disable later.

---

# Non-Blocking

Do not use:

```
delay()

while(WiFi.status()!=WL_CONNECTED)
```

Everything must use millis().

loop() must return immediately.

---

# Encapsulation

Private members must remain private.

Avoid global variables.

Avoid exposing implementation details.

---

# Thread/Event Safety

The event callback should perform minimal work.

Heavy processing belongs in loop().

---

# Future Compatibility

The API must be suitable for future modules.

Example:

FirebaseManager should only need:

```cpp
wifi.isConnected();

wifi.getState();

wifi.getRSSI();

wifi.getIPAddress();
```

FirebaseManager must never call WiFi.begin() directly.

---

# Reliability Requirements

This module is intended for a home automation smart switch that runs continuously.

It must recover automatically from:

* Router reboot
* Mobile hotspot turned off/on
* Temporary signal loss
* DHCP renewal
* AP restart
* Short WiFi outages

The device should reconnect automatically without requiring a power cycle.

---

# Code Quality

Follow modern C++ best practices.

Requirements:

* enum class instead of enum
* const correctness
* descriptive naming
* short functions
* no duplicated code
* single responsibility principle
* clear separation between public and private APIs

Readability is more important than minimizing line count.

The final result should resemble a reusable networking component that could be copied into any ESP32 IoT project without modification.
