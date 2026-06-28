# StatusLedManager

A reusable RGB Status LED manager for ESP32 using a single WS2812B / NeoPixel LED.

---

# Requirements

- ESP32
- Adafruit NeoPixel Library

---

# Installation

Copy these files into your project.

```text
Device/
├── StatusLedManager.h
└── StatusLedManager.cpp
```

Include the header.

```cpp
#include "Device/StatusLedManager.h"
```

---

# Create Object

```cpp
StatusLedManager led(5);
```

5 = GPIO connected to the NeoPixel DIN pin.

---

# Setup

```cpp
void setup()
{
    led.begin();
}
```

---

# Loop

Always call loop().

```cpp
void loop()
{
    led.loop();
}
```

---

# Power Control

Turn LED ON

```cpp
led.on();
```

Turn LED OFF

```cpp
led.off();
```

Enable LED

```cpp
led.setEnabled(true);
```

Disable LED

```cpp
led.setEnabled(false);
```

Check if enabled

```cpp
led.isEnabled();
```

---

# Brightness

Set brightness

```cpp
led.setBrightness(100);
```

Get brightness

```cpp
led.getBrightness();
```

Range

```text
0 - 255
```

---

# Colors

Using predefined colors

```cpp
led.setColor(LedColor::Red);

led.setColor(LedColor::Green);

led.setColor(LedColor::Blue);

led.setColor(LedColor::White);

led.setColor(LedColor::Yellow);

led.setColor(LedColor::Orange);

led.setColor(LedColor::Purple);

led.setColor(LedColor::Cyan);

led.setColor(LedColor::Pink);
```

Using custom RGB

```cpp
led.setColor(255, 50, 150);
```

Get current color

```cpp
led.getColor();
```

---

# Animations

Solid

```cpp
led.setAnimation(LedAnimation::Solid);
```

Blink

```cpp
led.setAnimation(LedAnimation::Blink);
```

Slow Blink

```cpp
led.setAnimation(LedAnimation::SlowBlink);
```

Fast Blink

```cpp
led.setAnimation(LedAnimation::FastBlink);
```

Breathing

```cpp
led.setAnimation(LedAnimation::Breathe);
```

Pulse

```cpp
led.pulse();
```

Flash

```cpp
led.flash(3);
```

Rainbow

```cpp
led.rainbow();
```

Stop animation

```cpp
led.stopAnimation();
```

Animation speed

```cpp
led.setAnimationSpeed(500);
```

---

# Device States

Instead of managing colors manually, use predefined states.

```cpp
led.setState(StatusLedState::Booting);

led.setState(StatusLedState::Idle);

led.setState(StatusLedState::Success);

led.setState(StatusLedState::Busy);

led.setState(StatusLedState::Warning);

led.setState(StatusLedState::Error);

led.setState(StatusLedState::Pairing);

led.setState(StatusLedState::Updating);

led.setState(StatusLedState::FactoryReset);
```

Get current state

```cpp
led.getState();
```

---

# Temporary Override

Temporarily show another state.

```cpp
led.overrideState(StatusLedState::Warning);
```

Restore previous state.

```cpp
led.clearOverride();
```

---

# Typical Usage

```cpp
#include "Device/StatusLedManager.h"

StatusLedManager led(5);

void setup()
{
    led.begin();

    led.setBrightness(80);

    led.setState(StatusLedState::Booting);
}

void loop()
{
    led.loop();

    // WiFi Connected
    led.setState(StatusLedState::Success);

    // WiFi Connecting
    led.setState(StatusLedState::Busy);

    // Error
    led.setState(StatusLedState::Error);
}
```

---

# Enumerations

## LedColor

```cpp
Off
Red
Green
Blue
White
Yellow
Orange
Purple
Cyan
Pink
Custom
```

---

## LedAnimation

```cpp
None
Solid
Blink
SlowBlink
FastBlink
Breathe
Pulse
Flash
Rainbow
```

---

## StatusLedState

```cpp
Off
Booting
Idle
Success
Busy
Warning
Error
Pairing
Updating
FactoryReset
Custom
```
