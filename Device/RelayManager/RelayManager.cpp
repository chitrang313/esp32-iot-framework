#include "RelayManager.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RelayManager::RelayManager()
    : m_relays{}            // zero-initialises all RelayEntry fields; configured = false
    , m_configuredCount(0)
    , m_enabled(true)
    , m_began(false)
    , m_callback(nullptr)
    , m_callbackContext(nullptr)
{
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

RelayResult RelayManager::configureRelay(RelayChannel     channel,
                                          uint8_t          gpioPin,
                                          RelayActiveState activeState,
                                          const char*      description)
{
    // Reject registration attempts that arrive after begin() seals configuration.
    if (m_began)
    {
        return RelayResult::ConfigurationClosed;
    }

    // Reject out-of-range channel values before using them as array indices.
    if (!isValidChannel(channel))
    {
        return RelayResult::InvalidChannel;
    }

    const uint8_t index = channelToIndex(channel);

    // Duplicate channel — same RelayChannel configured twice.
    if (m_relays[index].configured)
    {
        return RelayResult::AlreadyConfigured;
    }

    // Reject GPIO numbers beyond the physical range of the ESP32 classic.
    // Raise ESP32_MAX_GPIO_PIN for S2 (45) or S3 (48) variants.
    if (gpioPin > ESP32_MAX_GPIO_PIN)
    {
        return RelayResult::InvalidPin;
    }

    // Duplicate GPIO pin — same physical line wired to two channels.
    if (isPinAlreadyUsed(gpioPin))
    {
        return RelayResult::DuplicatePin;
    }

    // Store the per-relay configuration.
    RelayEntry& entry  = m_relays[index];
    entry.pin          = gpioPin;
    entry.activeState  = activeState;
    entry.state        = RelayState::Off;    // logical default before begin()
    entry.configured   = true;
    entry.enabled      = true;              // per-relay enable reserved for future use

    // Copy description with hard null-termination guard; never trusts caller length.
    strncpy(entry.description, description, MAX_DESCRIPTION_LEN - 1);
    entry.description[MAX_DESCRIPTION_LEN - 1] = '\0';

    m_configuredCount++;

    return RelayResult::Success;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RelayManager::begin()
{
    // Seal configuration first: subsequent configureRelay() calls return
    // ConfigurationClosed.  m_began must be true before writePin() is called
    // below — writePin() guards on it.
    m_began = true;

    // Initialise only the relays that were actually configured.  Unconfigured
    // slots never get a pinMode() call and are silently skipped.
    for (uint8_t index = 0; index < MAX_RELAYS; index++)
    {
        if (!m_relays[index].configured)
        {
            continue;
        }

        // Set OUTPUT first, then write the OFF level — prevents a momentary
        // glitch to whatever logic state the pin floated to at power-on.
        pinMode(m_relays[index].pin, OUTPUT);

        m_relays[index].state = RelayState::Off;
        writePin(index, RelayState::Off);
    }
}

void RelayManager::loop()
{
    // Intentionally empty for Phase 1.
    // Reserved for future per-tick work: timed pulses, auto-off timers,
    // deferred callbacks, persistence flush — all without changing the API.
}

// ---------------------------------------------------------------------------
// Enable / disable
// ---------------------------------------------------------------------------

void RelayManager::enable()
{
    m_enabled = true;
}

void RelayManager::disable()
{
    // Physical outputs intentionally left untouched — hardware holds last state.
    // Only mutating commands are rejected until enable() is called again.
    m_enabled = false;
}

bool RelayManager::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Single-channel control
// ---------------------------------------------------------------------------

RelayResult RelayManager::turnOn(RelayChannel channel)
{
    return setState(channel, RelayState::On);
}

RelayResult RelayManager::turnOff(RelayChannel channel)
{
    return setState(channel, RelayState::Off);
}

RelayResult RelayManager::toggle(RelayChannel channel)
{
    // Validate before reading internal state — out-of-range must never index array.
    if (!isValidChannel(channel))
    {
        return RelayResult::InvalidChannel;
    }

    const uint8_t index = channelToIndex(channel);

    if (!m_relays[index].configured)
    {
        return RelayResult::NotConfigured;
    }

    if (!m_enabled)
    {
        return RelayResult::Disabled;
    }

    const RelayState target =
        (m_relays[index].state == RelayState::On) ? RelayState::Off : RelayState::On;

    return setState(channel, target);
}

RelayResult RelayManager::setState(RelayChannel channel, RelayState state)
{
    // Validation order is documented in the header; do not reorder.
    if (!isValidChannel(channel))
    {
        return RelayResult::InvalidChannel;
    }

    const uint8_t index = channelToIndex(channel);

    if (!m_relays[index].configured)
    {
        return RelayResult::NotConfigured;
    }

    if (!m_enabled)
    {
        return RelayResult::Disabled;
    }

    if (m_relays[index].state == state)
    {
        // Skip the hardware write entirely — state is already correct.
        return (state == RelayState::On) ? RelayResult::AlreadyOn
                                         : RelayResult::AlreadyOff;
    }

    m_relays[index].state = state;
    writePin(index, state);
    notifyStateChanged(channel, state);

    return RelayResult::Success;
}

// ---------------------------------------------------------------------------
// Single-channel query
// ---------------------------------------------------------------------------

RelayState RelayManager::getState(RelayChannel channel) const
{
    if (!isValidChannel(channel) || !m_relays[channelToIndex(channel)].configured)
    {
        return RelayState::Off;     // safe default; never crashes on bad input
    }
    return m_relays[channelToIndex(channel)].state;
}

bool RelayManager::isOn(RelayChannel channel) const
{
    return getState(channel) == RelayState::On;
}

bool RelayManager::isOff(RelayChannel channel) const
{
    return getState(channel) == RelayState::Off;
}

// ---------------------------------------------------------------------------
// Bulk control
// ---------------------------------------------------------------------------

void RelayManager::turnAllOn()
{
    for (uint8_t index = 0; index < MAX_RELAYS; index++)
    {
        if (m_relays[index].configured)
        {
            setState(static_cast<RelayChannel>(index), RelayState::On);
        }
    }
}

void RelayManager::turnAllOff()
{
    for (uint8_t index = 0; index < MAX_RELAYS; index++)
    {
        if (m_relays[index].configured)
        {
            setState(static_cast<RelayChannel>(index), RelayState::Off);
        }
    }
}

void RelayManager::toggleAll()
{
    for (uint8_t index = 0; index < MAX_RELAYS; index++)
    {
        if (m_relays[index].configured)
        {
            toggle(static_cast<RelayChannel>(index));
        }
    }
}

// ---------------------------------------------------------------------------
// Information queries
// ---------------------------------------------------------------------------

uint8_t RelayManager::getRelayCount() const
{
    return m_configuredCount;
}

bool RelayManager::isConfigured(RelayChannel channel) const
{
    if (!isValidChannel(channel))
    {
        return false;
    }
    return m_relays[channelToIndex(channel)].configured;
}

uint8_t RelayManager::getRelayPin(RelayChannel channel) const
{
    if (!isValidChannel(channel) || !m_relays[channelToIndex(channel)].configured)
    {
        return INVALID_GPIO_PIN;    // 0xFF: caller may compare against this sentinel
    }
    return m_relays[channelToIndex(channel)].pin;
}

const char* RelayManager::getRelayDescription(RelayChannel channel) const
{
    if (!isValidChannel(channel) || !m_relays[channelToIndex(channel)].configured)
    {
        return "";
    }
    return m_relays[channelToIndex(channel)].description;
}

// ---------------------------------------------------------------------------
// State-change callback
// ---------------------------------------------------------------------------

void RelayManager::onStateChanged(StateChangedCallback callback, void* context)
{
    m_callback        = callback;
    m_callbackContext = context;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

uint8_t RelayManager::channelToIndex(RelayChannel channel)
{
    // RelayChannel enum values equal their zero-based array indices by design.
    return static_cast<uint8_t>(channel);
}

bool RelayManager::isValidChannel(RelayChannel channel) const
{
    // Validates range only.  Separate from the configured check so callers can
    // distinguish "invalid channel number" from "channel not configured".
    return channelToIndex(channel) < MAX_RELAYS;
}

bool RelayManager::isPinAlreadyUsed(uint8_t gpioPin) const
{
    // Scan only configured entries — unconfigured slots hold zero-initialised
    // pin values which could coincidentally match a real GPIO number.
    for (uint8_t index = 0; index < MAX_RELAYS; index++)
    {
        if (m_relays[index].configured && m_relays[index].pin == gpioPin)
        {
            return true;
        }
    }
    return false;
}

uint8_t RelayManager::physicalLevelFor(RelayActiveState activeState, RelayState state)
{
    const bool energised = (state == RelayState::On);

    // ActiveHigh: On → HIGH,  Off → LOW
    // ActiveLow:  On → LOW,   Off → HIGH
    if (activeState == RelayActiveState::ActiveHigh)
    {
        return energised ? HIGH : LOW;
    }
    return energised ? LOW : HIGH;
}

void RelayManager::writePin(uint8_t index, RelayState state)
{
    // Guard against writes before begin() has called pinMode().
    // Callers are responsible for ensuring index is valid and configured.
    if (!m_began)
    {
        return;
    }
    digitalWrite(m_relays[index].pin,
                 physicalLevelFor(m_relays[index].activeState, state));
}

void RelayManager::notifyStateChanged(RelayChannel channel, RelayState newState)
{
    if (m_callback != nullptr)
    {
        m_callback(channel, newState, m_callbackContext);
    }
}
