# InputManager - instruction.md

# Objective

Develop a reusable, production-quality InputManager for ESP32.

InputManager is responsible for handling all digital input devices.

Examples:

- TTP223 Touch Sensor
- Push Button
- Toggle Switch
- Reed Switch
- Magnetic Sensor
- PIR Motion Sensor
- Float Switch
- Custom Digital Inputs

InputManager must never know about:

- RelayManager
- StatusLedManager
- IoTWiFiManager
- FirebaseManager
- PreferencesManager

Its only responsibility is detecting input events.

---

# Deliverables

Create only these files.

```text
Input/
└── InputManager/
    ├── InputManager.h
    ├── InputManager.cpp
    ├── README.md
    └── instruction.md
```

No example sketch.

No relay control.

No WiFi.

No Firebase.

No LED.

Only InputManager.

---

# Design Principles

Follow:

- Single Responsibility Principle
- Open / Closed Principle
- Clean Architecture
- Modern C++
- Encapsulation

InputManager owns all GPIO operations related to digital inputs.

---

# Input Capacity

Support a configurable compile-time maximum.

Example

```cpp
MAX_INPUTS
```

Default

```text
16
```

Application may configure any number of inputs from

```text
1 ... MAX_INPUTS
```

---

# Configuration

The constructor should not require any configuration.

Example

```cpp
InputManager input;
```

Before begin(), every input must be configured individually.

Provide an API similar to

```cpp
configureInput(
    InputChannel channel,
    uint8_t gpioPin,
    InputType type,
    InputMode mode,
    const char* description
);
```

Parameters

- InputChannel
- GPIO Pin
- Input Type
- Input Mode
- Description

The description is stored internally for future diagnostics, web UI, cloud integration and logging.

Phase 1 does not use the description.

---

# Configuration Rules

- configureInput() must be called before begin()
- Duplicate channels are rejected
- Duplicate GPIO pins are rejected
- Invalid GPIO pins are rejected
- Input count must never exceed MAX_INPUTS
- begin() initializes only configured inputs
- Inputs not configured remain unavailable

---

# Example Usage

```cpp
InputManager input;

void setup()
{
    input.configureInput(
        InputChannel::Input1,
        13,
        InputType::TTP223,
        InputMode::PullDown,
        "Kitchen Touch"
    );

    input.configureInput(
        InputChannel::Input2,
        14,
        InputType::PushButton,
        InputMode::PullUp,
        "Bedroom Button"
    );

    input.begin();
}
```

Changing ESP32 boards should only require changing GPIO numbers inside configureInput().

---

# Enumerations

Use enum class wherever possible.

## InputChannel

Input1

Input2

...

Input16

---

## InputType

PushButton

ToggleSwitch

TTP223

ReedSwitch

MagneticSensor

PIR

FloatSwitch

Custom

---

## InputMode

PullUp

PullDown

Floating

---

## InputState

Pressed

Released

---

## InputEvent

Pressed

Released

Click

DoubleClick

LongPress

VeryLongPress

Hold

---

## InputResult

Success

InvalidChannel

AlreadyConfigured

DuplicatePin

InvalidPin

NotConfigured

Disabled

ConfigurationClosed

---

# Public API

Lifecycle

- begin()
- loop()
- enable()
- disable()
- isEnabled()

Configuration

- configureInput(...)

Information

- isConfigured(...)
- getInputCount()
- getInputPin(...)
- getDescription(...)

Input State

- isPressed(...)
- isReleased(...)
- getState(...)

Callbacks

- onPressed(...)
- onReleased(...)
- onClick(...)
- onDoubleClick(...)
- onLongPress(...)
- onVeryLongPress(...)
- onHold(...)

---

# Phase 1 Features

Implement architecture for:

- Press detection
- Release detection
- Debounce
- Callback system

---

# Future Features

Architecture must support without API changes:

- Double Click
- Triple Click
- Long Press
- Very Long Press
- Hold
- Auto Repeat
- Press Duration
- Interrupt Mode
- Polling Mode
- Enable / Disable Individual Inputs
- Statistics
- Event Queue
- Event History
- Input Groups

Implementation is not required in Phase 1.

---

# Internal Storage

Each configured input should internally maintain

- Input Channel
- GPIO Pin
- Description
- Input Type
- Input Mode
- Current State
- Previous State
- Last Change Time
- Debounce Timer
- Configured Flag
- Enabled Flag

Application code must never access these members directly.

---

# GPIO Ownership

Only InputManager may call

- pinMode()
- digitalRead()

No other module should access input GPIO pins.

---

# Debounce

Debouncing belongs entirely inside InputManager.

Application code should never implement debounce logic.

Future input types should automatically benefit from the same debounce implementation.

---

# Non-Blocking

Never use

- delay()
- while()

All timing must use millis().

loop() should execute quickly.

---

# Logging

InputManager must never use

- Serial
- Serial.println()

Logging belongs to a future Logger module.

---

# Memory

Avoid

- new
- delete
- String

Prefer fixed-size storage.

Suitable for 24/7 embedded systems.

---

# Future Compatibility

InputManager must not know about any other module.

Instead, it publishes events.

Example

```text
Touch Pressed

↓

InputManager

↓

Callback

↓

Application

↓

RelayManager
```

or

```text
Touch Pressed

↓

Application

↓

FirebaseManager
```

or

```text
Touch Pressed

↓

Application

↓

StatusLedManager
```

InputManager never communicates directly with other managers.

---

# Coding Standards

Use

- enum class
- const correctness
- descriptive names
- helper functions
- private members
- no duplicated code
- short functions

Readable code is preferred.

---

# Framework Standard

Every manager follows the same lifecycle.

```cpp
begin();

loop();

enable();

disable();

isEnabled();
```

InputManager must follow the same convention.

---

# Final Goal

Application code should never call

```cpp
pinMode()

digitalRead()
```

Instead, the application configures each input once.

```cpp
input.configureInput(
    InputChannel::Input1,
    13,
    InputType::TTP223,
    InputMode::PullDown,
    "Kitchen Touch"
);
```

After configuration, the application only reacts to events.

```cpp
input.onPressed(onInputPressed);

void onInputPressed(InputChannel channel)
{
    // Application decides what to do.
}
```

InputManager becomes the single source of truth for all digital inputs across every ESP32 project.
