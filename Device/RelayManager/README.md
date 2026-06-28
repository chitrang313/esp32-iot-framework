# RelayManager

Reusable relay controller for ESP32.

Supports:

- Configurable relay mapping
- 1 to MAX_RELAYS
- Active HIGH / Active LOW relays
- Internal relay state management
- Non-blocking operation
- State change callbacks

---

# Installation

Copy the module into your project.

```text
Device/
└── RelayManager/
    ├── RelayManager.h
    ├── RelayManager.cpp
    ├── README.md
    └── instruction.md
```

Include the header.

```cpp
#include "Device/RelayManager/RelayManager.h"
```

---

# Create Object

```cpp
RelayManager relay;
```

---

# Configure Relays

Configure each relay before calling `begin()`.

```cpp
relay.configureRelay(
    RelayChannel::Relay1,
    23,
    RelayActiveState::ActiveLow,
    "Kitchen Light"
);

relay.configureRelay(
    RelayChannel::Relay2,
    5,
    RelayActiveState::ActiveLow,
    "Bedroom Light"
);

relay.configureRelay(
    RelayChannel::Relay3,
    18,
    RelayActiveState::ActiveHigh,
    "Exhaust Fan"
);
```

Only configure the relays used by your hardware.

---

# Setup

```cpp
void setup()
{
    relay.begin();
}
```

---

# Loop

Always call `loop()`.

```cpp
void loop()
{
    relay.loop();
}
```

---

# Enable / Disable

```cpp
relay.enable();

relay.disable();

relay.isEnabled();
```

---

# Turn Relay ON

```cpp
relay.turnOn(RelayChannel::Relay1);
```

---

# Turn Relay OFF

```cpp
relay.turnOff(RelayChannel::Relay1);
```

---

# Toggle Relay

```cpp
relay.toggle(RelayChannel::Relay1);
```

---

# Set Relay State

```cpp
relay.setState(
    RelayChannel::Relay1,
    RelayState::On
);

relay.setState(
    RelayChannel::Relay1,
    RelayState::Off
);
```

---

# Get Relay State

```cpp
RelayState state =
    relay.getState(
        RelayChannel::Relay1
    );
```

---

# Check Relay State

```cpp
relay.isOn(RelayChannel::Relay1);

relay.isOff(RelayChannel::Relay1);
```

---

# Control All Relays

```cpp
relay.turnAllOn();

relay.turnAllOff();

relay.toggleAll();
```

---

# Information

Configured relay count.

```cpp
relay.getRelayCount();
```

Check whether a relay is configured.

```cpp
relay.isConfigured(
    RelayChannel::Relay1
);
```

Get configured GPIO pin.

```cpp
relay.getRelayPin(
    RelayChannel::Relay1
);
```

Get relay description.

```cpp
relay.getRelayDescription(
    RelayChannel::Relay1
);
```

---

# State Change Callback

Register a callback.

```cpp
relay.onStateChanged(
    onRelayStateChanged
);
```

Example.

```cpp
void onRelayStateChanged(
    RelayChannel channel,
    RelayState state,
    void* context
)
{
    // Handle relay change
}
```

---

# Operation Result

Every relay operation returns a `RelayResult`.

```cpp
RelayResult result =
    relay.turnOn(
        RelayChannel::Relay1
    );

if(result == RelayResult::Success)
{
    // Relay turned ON
}
```

---

# Enumerations

## RelayChannel

```cpp
Relay1
Relay2
...
Relay16
```

---

## RelayState

```cpp
Off

On
```

---

## RelayActiveState

```cpp
ActiveHigh

ActiveLow
```

---

## RelayResult

```cpp
Success

InvalidChannel

AlreadyOn

AlreadyOff

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
#include "Device/RelayManager/RelayManager.h"

RelayManager relay;

void setup()
{
    relay.configureRelay(
        RelayChannel::Relay1,
        23,
        RelayActiveState::ActiveLow,
        "Kitchen Light"
    );

    relay.configureRelay(
        RelayChannel::Relay2,
        5,
        RelayActiveState::ActiveLow,
        "Bedroom Light"
    );

    relay.begin();

    relay.turnOn(RelayChannel::Relay1);
}

void loop()
{
    relay.loop();

    relay.toggle(RelayChannel::Relay2);
}
```

---

# Best Practices

- Configure all relays before calling `begin()`.
- Use `RelayChannel` instead of GPIO numbers.
- Never call `digitalWrite()` outside `RelayManager`.
- Keep all relay operations inside `RelayManager`.
- Check `RelayResult` when command success matters.
- Change GPIO mappings only in `configureRelay()` when using different ESP32 boards.
