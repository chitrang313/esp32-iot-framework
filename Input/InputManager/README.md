# InputManager

Reusable digital input manager for ESP32.

Supports:

- TTP223 Touch Sensor
- Push Button
- Toggle Switch
- Reed Switch
- Magnetic Sensor
- PIR Motion Sensor
- Float Switch
- Custom Digital Inputs

Features:

- Configurable input mapping
- Internal debounce
- Non-blocking operation
- State change callbacks
- Configurable active level
- Supports up to MAX_INPUTS

---

# Installation

Copy the module into your project.

```text
Input/
└── InputManager/
    ├── InputManager.h
    ├── InputManager.cpp
    ├── README.md
    └── instruction.md
```

Include the header.

```cpp
#include "Input/InputManager/InputManager.h"
```

---

# Create Object

```cpp
InputManager input;
```

---

# Configure Inputs

Configure every input before calling `begin()`.

```cpp
input.configureInput(
    InputChannel::Input1,
    13,
    InputType::TTP223,
    InputMode::PullDown,
    InputActiveState::ActiveHigh,
    "Kitchen Touch"
);

input.configureInput(
    InputChannel::Input2,
    14,
    InputType::PushButton,
    InputMode::PullUp,
    InputActiveState::ActiveLow,
    "Bedroom Button"
);
```

Only configure the inputs used by your hardware.

---

# Setup

```cpp
void setup()
{
    input.begin();
}
```

---

# Loop

Always call `loop()`.

```cpp
void loop()
{
    input.loop();
}
```

---

# Enable / Disable

```cpp
input.enable();

input.disable();

input.isEnabled();
```

---

# Input State

Check whether an input is pressed.

```cpp
input.isPressed(InputChannel::Input1);
```

Check whether an input is released.

```cpp
input.isReleased(InputChannel::Input1);
```

Get current input state.

```cpp
InputState state =
    input.getState(
        InputChannel::Input1
    );
```

---

# Information

Get configured input count.

```cpp
input.getInputCount();
```

Check whether an input is configured.

```cpp
input.isConfigured(
    InputChannel::Input1
);
```

Get configured GPIO pin.

```cpp
input.getInputPin(
    InputChannel::Input1
);
```

Get input description.

```cpp
input.getDescription(
    InputChannel::Input1
);
```

---

# Callbacks

Pressed

```cpp
input.onPressed(onPressed);
```

Released

```cpp
input.onReleased(onReleased);
```

Click

```cpp
input.onClick(onClick);
```

Double Click

```cpp
input.onDoubleClick(onDoubleClick);
```

Long Press

```cpp
input.onLongPress(onLongPress);
```

Very Long Press

```cpp
input.onVeryLongPress(onVeryLongPress);
```

Hold

```cpp
input.onHold(onHold);
```

---

# Callback Example

```cpp
void onPressed(
    InputChannel channel,
    void* context
)
{
    // Application decides what to do
}
```

---

# Return Result

Configuration returns an `InputResult`.

```cpp
InputResult result =
    input.configureInput(
        InputChannel::Input1,
        13,
        InputType::TTP223,
        InputMode::PullDown,
        InputActiveState::ActiveHigh,
        "Kitchen Touch"
    );

if(result == InputResult::Success)
{
    // Input configured
}
```

---

# Enumerations

## InputChannel

```cpp
Input1
Input2
...
Input16
```

---

## InputType

```cpp
PushButton

ToggleSwitch

TTP223

ReedSwitch

MagneticSensor

PIR

FloatSwitch

Custom
```

---

## InputMode

```cpp
PullUp

PullDown

Floating
```

---

## InputActiveState

```cpp
ActiveHigh

ActiveLow
```

---

## InputState

```cpp
Pressed

Released
```

---

## InputEvent

```cpp
Pressed

Released

Click

DoubleClick

LongPress

VeryLongPress

Hold
```

---

## InputResult

```cpp
Success

InvalidChannel

AlreadyConfigured

DuplicatePin

InvalidPin

NotConfigured

Disabled

ConfigurationClosed
```

---

# Complete Example

```cpp
#include "Input/InputManager/InputManager.h"

InputManager input;

void setup()
{
    input.configureInput(
        InputChannel::Input1,
        13,
        InputType::TTP223,
        InputMode::PullDown,
        InputActiveState::ActiveHigh,
        "Kitchen Touch"
    );

    input.begin();

    input.onPressed(onPressed);
}

void loop()
{
    input.loop();
}

void onPressed(
    InputChannel channel,
    void* context
)
{
    // Handle input event
}
```

---

# Best Practices

- Configure all inputs before calling `begin()`.
- Always call `loop()`.
- Use callbacks instead of polling whenever possible.
- Never call `digitalRead()` outside `InputManager`.
- Let `InputManager` handle debounce.
- Keep application logic outside `InputManager`.
- InputManager should never know about RelayManager, WiFi, Firebase, or LED modules.
