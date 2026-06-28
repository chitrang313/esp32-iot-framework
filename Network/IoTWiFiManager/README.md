# IoTWiFiManager

A reusable, production-oriented WiFi manager for ESP32 projects.

The purpose of this module is to provide reliable WiFi connectivity for long-running IoT devices such as:

- Smart Switch
- Smart Plug
- Smart Fan
- Smart Lock
- Weather Station
- Home Automation Controller

This module is completely independent from Firebase, MQTT, LEDs, OTA, or application logic.

---

# Features

- Event-driven architecture
- Automatic WiFi reconnection
- Exponential reconnect backoff
- Non-blocking implementation
- Connection state machine
- Connection timeout handling
- Disconnect reason tracking
- Connection duration tracking
- RSSI monitoring
- IP Address retrieval
- Clean and reusable API
- Suitable for 24/7 IoT devices

---

# Requirements

- ESP32 Arduino Core 3.x
- Built-in WiFi library

No additional dependencies.

---

# Installation

Copy the following files into your project.

```text
Network/
├── IoTWiFiManager.h
└── IoTWiFiManager.cpp
```

---

# Include

```cpp
#include "Network/IoTWiFiManager.h"
```

---

# Create Instance

```cpp
IoTWiFiManager wifi(
    "YourSSID",
    "YourPassword"
);
```

---

# Setup

Call begin() once.

```cpp
void setup()
{
    Serial.begin(115200);

    wifi.begin();
}
```

---

# Loop

Call loop() continuously.

```cpp
void loop()
{
    wifi.loop();
}
```

The manager is completely non-blocking.

---

# Check Connection

```cpp
if (wifi.isConnected())
{
    Serial.println("WiFi Connected");
}
else
{
    Serial.println("WiFi Disconnected");
}
```

---

# Get Current State

```cpp
switch (wifi.getState())
{
    case WiFiState::Connected:
        break;

    case WiFiState::Connecting:
        break;

    case WiFiState::Reconnecting:
        break;

    case WiFiState::Disconnected:
        break;

    default:
        break;
}
```

---

# Get IP Address

```cpp
Serial.println(wifi.getIPAddress());
```

---

# Get RSSI

```cpp
Serial.println(wifi.getRSSI());
```

---

# Get Connected SSID

```cpp
Serial.println(wifi.getSSID());
```

---

# Get Connected Duration

Returns the number of seconds the device has remained continuously connected.

```cpp
Serial.println(
    wifi.getConnectedDuration()
);
```

---

# Get Reconnect Count

Returns the total number of reconnect attempts.

```cpp
Serial.println(
    wifi.getReconnectCount()
);
```

---

# Get Disconnect Reason

Useful for debugging.

```cpp
Serial.println(
    wifi.getLastDisconnectReason()
);
```

---

# Manual Reconnect

Force a reconnect.

```cpp
wifi.reconnect();
```

---

# Manual Disconnect

Disconnect from WiFi.

```cpp
wifi.disconnect();
```

---

# Typical Usage

```cpp
#include "Network/IoTWiFiManager.h"

IoTWiFiManager wifi(
    "HomeWiFi",
    "password123"
);

void setup()
{
    Serial.begin(115200);

    wifi.begin();
}

void loop()
{
    wifi.loop();

    if (wifi.isConnected())
    {
        // Firebase
        // MQTT
        // HTTP
        // OTA
    }

    // Other application logic
}
```

---

# Design Philosophy

This module owns **all WiFi responsibilities**.

It is responsible for:

- Connecting
- Disconnecting
- Automatic reconnection
- WiFi events
- Connection state
- Diagnostics

It is **not** responsible for:

- Firebase
- MQTT
- LEDs
- Relays
- Buttons
- Sensors
- OTA Updates

Those should be implemented in their own dedicated modules.

---

# Recommended Architecture

```text
Application
│
├── RelayManager
├── ButtonManager
├── FirebaseManager
├── MQTTManager
├── LedManager
│
└── IoTWiFiManager
        │
        └── ESP32 WiFi Driver
```

All higher-level modules should query the WiFi manager.

Example:

```cpp
if (wifi.isConnected())
{
    firebase.loop();
}
```

Never call:

```cpp
WiFi.begin(...)
```

outside IoTWiFiManager.

---

# Best Practices

- Call begin() only once.
- Call loop() every iteration.
- Never block the loop with delay().
- Never directly use WiFi.begin() elsewhere in your project.
- Let IoTWiFiManager own the complete WiFi lifecycle.
- Keep networking separated from cloud services.

---

# Future Modules

IoTWiFiManager is intended to be the networking foundation for future reusable modules.

Examples:

- FirebaseManager
- MQTTManager
- LedManager
- OTAUpdateManager
- RelayManager
- ButtonManager

These modules should only depend on the public API exposed by IoTWiFiManager.

---

# License

This module is intended to be reused across multiple ESP32 projects and can serve as the networking foundation of a personal IoT framework.
