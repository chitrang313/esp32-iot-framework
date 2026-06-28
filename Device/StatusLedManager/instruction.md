# instruction.md

# StatusLedManager - Phase 1

## Objective

Develop a reusable, production-quality Status LED Manager for ESP32.

This module will become the standard LED driver for all future ESP32 projects.

Examples:

- Smart Switch
- Smart Plug
- Smart Fan
- Smart Door Lock
- Weather Station
- Home Automation Controller

The module must be completely independent from all other modules.

It must NEVER know anything about:

- WiFi
- Firebase
- MQTT
- OTA
- Relay
- Button
- Sensors

Its only responsibility is controlling one RGB Status LED.

---

# Deliverables

Create only these files.

```text
Device/
├── StatusLedManager.h
└── StatusLedManager.cpp
```

Nothing else.

---

# Target Hardware

ESP32

One WS2812B / NeoPixel RGB LED

Adafruit NeoPixel Library

Only one LED for Phase 1.

The architecture should allow future support for multiple LEDs without changing the public API.

---

# Design Principles

Follow:

- Single Responsibility Principle
- Open/Closed Principle
- Encapsulation
- Clean Architecture
- Modern C++

The module must be reusable.

No application-specific logic.

---

# Public API

Design a clean API similar to:

```cpp
class StatusLedManager
{
public:

    explicit StatusLedManager(uint8_t gpioPin);

    void begin();

    void loop();

    void on();

    void off();

    void setEnabled(bool enabled);

    bool isEnabled() const;

    void setBrightness(uint8_t brightness);

    uint8_t getBrightness() const;

    void setColor(LedColor color);

    void setColor(uint8_t r,
                  uint8_t g,
                  uint8_t b);

    LedColor getColor() const;

    void setAnimation(LedAnimation animation);

    LedAnimation getAnimation() const;

    void setAnimationSpeed(uint16_t ms);

    void stopAnimation();

    void pulse();

    void breathe();

    void blink();

    void blink(uint16_t interval);

    void flash(uint8_t count);

    void rainbow();

    void setState(StatusLedState state);

    StatusLedState getState() const;

    void overrideState(StatusLedState state);

    void clearOverride();

};
```

---

# Enumerations

Use enum class everywhere possible.

## LedColor

```cpp
enum class LedColor
{
    Off,

    Red,
    Green,
    Blue,

    White,

    Yellow,

    Orange,

    Purple,

    Cyan,

    Pink,

    Custom
};
```

---

## LedAnimation

```cpp
enum class LedAnimation
{
    None,

    Solid,

    Blink,

    SlowBlink,

    FastBlink,

    Breathe,

    Pulse,

    Flash,

    Rainbow
};
```

---

## StatusLedState

```cpp
enum class StatusLedState
{
    Off,

    Booting,

    Idle,

    Success,

    Busy,

    Warning,

    Error,

    Pairing,

    Updating,

    FactoryReset,

    Custom
};
```

---

# Supported Features

The architecture must support all of the following.

## 1

Turn LED ON

## 2

Turn LED OFF

## 3

Enable / Disable LED

## 4

Set Brightness

## 5

Get Brightness

## 6

Set predefined color

## 7

Set custom RGB color

## 8

Get current color

## 9

Solid animation

## 10

Blink animation

## 11

Slow Blink

## 12

Fast Blink

## 13

Breathing animation

## 14

Pulse animation

## 15

Flash animation

## 16

Rainbow animation

## 17

Stop animation

## 18

Animation speed

## 19

Device state abstraction

## 20

Temporary override state

## 21

Restore previous state

## 22

Query current state

## 23

Remember previous color

## 24

Future support for multiple LEDs

## 25

Fully non-blocking implementation

---

# Default State Mapping

StatusLedState should automatically configure color and animation.

Booting

Blue + Breathing

Idle

Off

Success

Green Solid

Busy

Blue Breathing

Warning

Orange Blink

Error

Red Fast Blink

Pairing

Yellow Blink

Updating

Purple Breathing

FactoryReset

White Fast Blink

Custom

Application controls manually.

---

# Animation Engine

Animations must NEVER use

delay()

Everything must run using

millis()

Supported:

Solid

Blink

Fast Blink

Slow Blink

Breathe

Pulse

Flash

Rainbow

loop() updates animations.

---

# Color Management

RGB values belong only inside StatusLedManager.

Application should prefer

```cpp
led.setColor(LedColor::Green);
```

instead of

```cpp
led.setColor(0,255,0);
```

---

# Brightness

Range

0-255

Default

50

Changing brightness must immediately affect the current animation.

---

# Override System

Support temporary override.

Example

WiFi Connected

↓

Green

↓

Button Press

↓

Blue Pulse

↓

Automatically restore Green

---

# Animation Queue

Design the architecture so animation queuing can be added later without changing the API.

Implementation may be deferred.

---

# Future Compatibility

The API should support future expansion to

- Multiple LEDs
- RGBW LEDs
- APA102
- Built-in RGB LEDs
- Addressable LED strips

without changing application code.

---

# Performance

No blocking.

No unnecessary NeoPixel updates.

Only update hardware when required.

loop() should execute quickly.

---

# Memory

Avoid dynamic allocation.

Avoid String.

Prefer fixed-size members.

Suitable for long-running embedded systems.

---

# Logging

Do not use Serial.

StatusLedManager should not produce logs.

---

# Encapsulation

Application code must NEVER call

```cpp
strip.setPixelColor(...)
```

or

```cpp
strip.show()
```

Those belong exclusively inside StatusLedManager.

---

# Code Quality

Requirements

- enum class
- const correctness
- private members
- descriptive naming
- short functions
- helper methods
- no duplicated code

Readable code is more important than fewer lines.

---

# Design Goal

The application should never think about RGB values or animations.

Instead, it should simply describe the desired state.

Example

```cpp
led.setState(StatusLedState::Busy);

led.setState(StatusLedState::Success);

led.setState(StatusLedState::Warning);

led.setState(StatusLedState::Error);
```

StatusLedManager decides:

- color
- animation
- timing
- brightness
- transitions

The module should be reusable across all ESP32 projects for many years without requiring API changes.
