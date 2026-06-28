# LoggerManager

A reusable logging manager for the ESP32 IoT Framework.

LoggerManager provides centralized logging for the framework and applications.

It receives events from EventBus, formats log messages, and outputs them to one or more logging targets.

---

# Features

* Centralized logging
* Log levels
* Event logging
* Timestamp support
* Serial output
* Custom output callback
* Log statistics
* Non-blocking design
* Framework-friendly architecture

---

# Create the manager

```cpp
LoggerManager logger;
```

---

# Initialize

```cpp
logger.begin();
```

---

# Log Messages

```cpp
logger.debug("Initializing WiFi...");

logger.info("WiFi Connected");

logger.warning("Low Signal Strength");

logger.error("Firebase Authentication Failed");
```

---

# Configure Log Level

```cpp
logger.setLogLevel(LogLevel::Info);

LogLevel level =
    logger.getLogLevel();
```

Only messages at or above the configured log level are output.

---

# Timestamp

Enable timestamps

```cpp
logger.enableTimestamp();
```

Disable timestamps

```cpp
logger.disableTimestamp();
```

Example output

```text
[12345][INFO] WiFi Connected

[12680][WARNING] Weak WiFi Signal

[12850][ERROR] Firebase Authentication Failed
```

---

# EventBus Integration

LoggerManager subscribes to EventBus.

Example

```text
RelayManager

↓

EventBus

↓

LoggerManager

↓

Serial Output
```

Framework modules do not need to call LoggerManager directly.

---

# Custom Output Callback

```cpp
logger.setOutputCallback(customLoggerCallback);
```

Allows applications to redirect logs to another destination.

Examples

* SD Card
* SPIFFS
* LittleFS
* Firebase
* MQTT
* Display

---

# Statistics

```cpp
uint32_t count =
    logger.getLogCount();

logger.clearLogCount();
```

---

# Main Loop

```cpp
void loop()
{
    logger.loop();
}
```

---

# Framework Lifecycle

```cpp
logger.begin();

logger.loop();

logger.enable();

logger.disable();

logger.isEnabled();
```

---

# Notes

* LoggerManager is non-blocking.
* Messages below the configured log level are ignored.
* LoggerManager receives framework events through EventBus.
* Serial output is the default logging destination.
* Additional output targets can be added without changing the public API.
* LoggerManager never performs application logic.
* LoggerManager is intended to be the single logging system for the entire framework.
