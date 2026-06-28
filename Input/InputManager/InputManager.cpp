#include "InputManager.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

InputManager::InputManager()
    : m_inputs{}          // zero-initialises all InputEntry fields; configured = false
    , m_configuredCount(0)
    , m_enabled(true)
    , m_began(false)
    , m_pressedCb      {nullptr, nullptr}
    , m_releasedCb     {nullptr, nullptr}
    , m_clickCb        {nullptr, nullptr}
    , m_doubleClickCb  {nullptr, nullptr}
    , m_longPressCb    {nullptr, nullptr}
    , m_veryLongPressCb{nullptr, nullptr}
    , m_holdCb         {nullptr, nullptr}
{
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

InputResult InputManager::configureInput(InputChannel     channel,
                                          uint8_t          gpioPin,
                                          InputType        type,
                                          InputMode        mode,
                                          InputActiveState activeState,
                                          const char*      description)
{
    // Reject any configuration that arrives after begin() has sealed the map.
    if (m_began)
    {
        return InputResult::ConfigurationClosed;
    }

    if (!isValidChannel(channel))
    {
        return InputResult::InvalidChannel;
    }

    const uint8_t index = channelToIndex(channel);

    if (m_inputs[index].configured)
    {
        return InputResult::AlreadyConfigured;
    }

    // Raise ESP32_MAX_GPIO_PIN for S2 (45) or S3 (48) variants.
    if (gpioPin > ESP32_MAX_GPIO_PIN)
    {
        return InputResult::InvalidPin;
    }

    // Scan only configured entries — unconfigured slots hold pin = 0 (zero-init)
    // which would falsely match a legitimate GPIO 0 assignment.
    if (isPinAlreadyUsed(gpioPin))
    {
        return InputResult::DuplicatePin;
    }

    InputEntry& entry       = m_inputs[index];
    entry.pin               = gpioPin;
    entry.type              = type;
    entry.mode              = mode;
    entry.activeState       = activeState;
    entry.state             = InputState::Released;
    entry.previousState     = InputState::Released;
    entry.pendingState      = InputState::Released;
    entry.debounceStartMs   = 0;
    entry.lastChangeMs      = 0;
    entry.configured        = true;
    entry.enabled           = true;     // per-input enable reserved for future use

    strncpy(entry.description, description, MAX_DESCRIPTION_LEN - 1);
    entry.description[MAX_DESCRIPTION_LEN - 1] = '\0';

    m_configuredCount++;

    return InputResult::Success;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void InputManager::begin()
{
    // Seal first so readRawState() (which calls digitalRead) can be called
    // inside the init loop — m_began guards nothing here but keeps the flag
    // semantically consistent.  configureInput() checks m_began to reject
    // late registrations.
    m_began = true;

    const uint32_t nowMs = millis();

    for (uint8_t index = 0; index < MAX_INPUTS; index++)
    {
        if (!m_inputs[index].configured)
        {
            continue;
        }

        InputEntry& entry = m_inputs[index];

        // Set the pull resistor mode BEFORE reading the pin — the GPIO level
        // is meaningless until the pull network has settled.
        pinMode(entry.pin, arduinoPinMode(entry.mode));

        // Seed both state and pendingState from the actual GPIO level so the
        // first loop() call does not fire a spurious Pressed or Released event
        // if the input is already active at power-on.
        const InputState initialState = readRawState(index);
        entry.state           = initialState;
        entry.previousState   = initialState;
        entry.pendingState    = initialState;
        entry.debounceStartMs = nowMs;
        entry.lastChangeMs    = nowMs;
    }
}

void InputManager::loop()
{
    if (!m_enabled)
    {
        return;
    }

    const uint32_t nowMs = millis();

    for (uint8_t index = 0; index < MAX_INPUTS; index++)
    {
        if (m_inputs[index].configured)
        {
            tickInput(index, nowMs);
        }
    }
}

// ---------------------------------------------------------------------------
// Enable / disable
// ---------------------------------------------------------------------------

void InputManager::enable()
{
    m_enabled = true;
}

void InputManager::disable()
{
    // GPIO pins stay configured; debounce state is preserved.
    // loop() skips processing while disabled so callbacks do not fire.
    m_enabled = false;
}

bool InputManager::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Information queries
// ---------------------------------------------------------------------------

bool InputManager::isConfigured(InputChannel channel) const
{
    if (!isValidChannel(channel))
    {
        return false;
    }
    return m_inputs[channelToIndex(channel)].configured;
}

uint8_t InputManager::getInputCount() const
{
    return m_configuredCount;
}

uint8_t InputManager::getInputPin(InputChannel channel) const
{
    if (!isValidChannel(channel) || !m_inputs[channelToIndex(channel)].configured)
    {
        return INVALID_GPIO_PIN;
    }
    return m_inputs[channelToIndex(channel)].pin;
}

const char* InputManager::getDescription(InputChannel channel) const
{
    if (!isValidChannel(channel) || !m_inputs[channelToIndex(channel)].configured)
    {
        return "";
    }
    return m_inputs[channelToIndex(channel)].description;
}

// ---------------------------------------------------------------------------
// Input state queries
// ---------------------------------------------------------------------------

InputState InputManager::getState(InputChannel channel) const
{
    if (!isValidChannel(channel) || !m_inputs[channelToIndex(channel)].configured)
    {
        return InputState::Released;    // safe default; never crashes on bad input
    }
    return m_inputs[channelToIndex(channel)].state;
}

bool InputManager::isPressed(InputChannel channel) const
{
    return getState(channel) == InputState::Pressed;
}

bool InputManager::isReleased(InputChannel channel) const
{
    return getState(channel) == InputState::Released;
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void InputManager::onPressed(InputEventCallback callback, void* context)
{
    setCallback(m_pressedCb, callback, context);
}

void InputManager::onReleased(InputEventCallback callback, void* context)
{
    setCallback(m_releasedCb, callback, context);
}

void InputManager::onClick(InputEventCallback callback, void* context)
{
    // Stored for Phase 2 activation; not fired in Phase 1.
    setCallback(m_clickCb, callback, context);
}

void InputManager::onDoubleClick(InputEventCallback callback, void* context)
{
    // Stored for Phase 2 activation; not fired in Phase 1.
    setCallback(m_doubleClickCb, callback, context);
}

void InputManager::onLongPress(InputEventCallback callback, void* context)
{
    // Stored for Phase 2 activation; not fired in Phase 1.
    setCallback(m_longPressCb, callback, context);
}

void InputManager::onVeryLongPress(InputEventCallback callback, void* context)
{
    // Stored for Phase 2 activation; not fired in Phase 1.
    setCallback(m_veryLongPressCb, callback, context);
}

void InputManager::onHold(InputEventCallback callback, void* context)
{
    // Stored for Phase 2 activation; not fired in Phase 1.
    setCallback(m_holdCb, callback, context);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

uint8_t InputManager::channelToIndex(InputChannel channel)
{
    // InputChannel enum values equal their zero-based indices by design.
    return static_cast<uint8_t>(channel);
}

bool InputManager::isValidChannel(InputChannel channel) const
{
    return channelToIndex(channel) < MAX_INPUTS;
}

bool InputManager::isPinAlreadyUsed(uint8_t gpioPin) const
{
    for (uint8_t index = 0; index < MAX_INPUTS; index++)
    {
        if (m_inputs[index].configured && m_inputs[index].pin == gpioPin)
        {
            return true;
        }
    }
    return false;
}

int InputManager::arduinoPinMode(InputMode mode)
{
    switch (mode)
    {
        case InputMode::PullUp:   return INPUT_PULLUP;
        // INPUT_PULLDOWN is ESP32-specific; not available on standard Arduino.
        case InputMode::PullDown: return INPUT_PULLDOWN;
        default:                  return INPUT;
    }
}

InputState InputManager::readRawState(uint8_t index) const
{
    const InputEntry& entry    = m_inputs[index];
    const int         gpioLevel = digitalRead(entry.pin);

    // Map the raw GPIO level to a logical state using the per-input polarity.
    // ActiveHigh: HIGH = Pressed;  ActiveLow: LOW = Pressed.
    if (entry.activeState == InputActiveState::ActiveHigh)
    {
        return (gpioLevel == HIGH) ? InputState::Pressed : InputState::Released;
    }
    return (gpioLevel == LOW) ? InputState::Pressed : InputState::Released;
}

void InputManager::tickInput(uint8_t index, uint32_t nowMs)
{
    InputEntry& entry = m_inputs[index];

    const InputState raw = readRawState(index);

    // Any change in the raw reading resets the debounce timer.  A glitch
    // that reverts within DEBOUNCE_MS is ignored entirely.
    if (raw != entry.pendingState)
    {
        entry.pendingState   = raw;
        entry.debounceStartMs = nowMs;
    }

    // Only accept the pending state once it has been stable for the full
    // debounce window AND it differs from the current confirmed state.
    if (entry.pendingState != entry.state
        && (nowMs - entry.debounceStartMs) >= DEBOUNCE_MS)
    {
        entry.previousState = entry.state;
        entry.state         = entry.pendingState;
        entry.lastChangeMs  = nowMs;

        const InputChannel channel = static_cast<InputChannel>(index);

        if (entry.state == InputState::Pressed)
        {
            fireCallback(m_pressedCb, channel);
        }
        else
        {
            fireCallback(m_releasedCb, channel);
        }

        // Click / LongPress / Hold detection uses lastChangeMs, previousState,
        // and pressStartMs (tracked by callers of lastChangeMs).
        // Phase 2 activates those callbacks here without changing this layout.
    }
}

void InputManager::fireCallback(const CallbackEntry& cb, InputChannel channel)
{
    if (cb.fn != nullptr)
    {
        cb.fn(channel, cb.context);
    }
}

void InputManager::setCallback(CallbackEntry&      entry,
                                InputEventCallback  callback,
                                void*               context)
{
    entry.fn      = callback;
    entry.context = context;
}
