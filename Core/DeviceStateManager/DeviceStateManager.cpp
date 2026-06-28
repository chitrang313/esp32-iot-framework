#include "DeviceStateManager.h"

// =============================================================================
// Construction
// =============================================================================

DeviceStateManager::DeviceStateManager()
    : m_initialized(false)
    , m_enabled(true)
    , m_uptimeStartMs(0u)
    , m_wifiState(WiFiState::Idle)
    , m_firebaseState(FirebaseState::NotConfigured)
    , m_deviceMode(DeviceMode::Booting)
    , m_online(false)
    , m_relayDirty(false)
    , m_switchDirty(false)
    , m_wifiDirty(false)
    , m_firebaseDirty(false)
    , m_systemDirty(false)
    , m_version(0u)
    , m_lastRelayUpdateMs(0u)
    , m_lastSwitchUpdateMs(0u)
    , m_lastWiFiUpdateMs(0u)
    , m_lastFirebaseUpdateMs(0u)
    , m_lastSystemUpdateMs(0u)
{
    // Arrays cannot be initialised in the member-initialiser list in C++11;
    // zero them here so the object is safe to query before begin() is called.
    for (uint8_t i = 0u; i < MAX_RELAYS; ++i) {
        m_relayStates[i] = RelayState::Off;
    }
    for (uint8_t i = 0u; i < MAX_SWITCHES; ++i) {
        m_switchStates[i] = SwitchState::Released;
    }
}

// =============================================================================
// Lifecycle
// =============================================================================

void DeviceStateManager::begin()
{
    // initState() resets runtime values to safe defaults.  Capture the uptime
    // start AFTER the reset so getUptime() measures from this begin() call.
    initState();
    m_uptimeStartMs = millis();
    m_initialized   = true;
}

void DeviceStateManager::loop()
{
    // Reserved for future background work (watchdog, state expiry).
    // No polling here — getters compute on demand; dirty flags need no upkeep.
}

void DeviceStateManager::enable()
{
    m_enabled = true;
}

void DeviceStateManager::disable()
{
    m_enabled = false;
}

bool DeviceStateManager::isEnabled() const
{
    return m_enabled;
}

// =============================================================================
// Relay state
// =============================================================================

DeviceStateResult DeviceStateManager::setRelayState(RelayChannel channel, RelayState state)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (!isValidRelayChannel(channel)) return DeviceStateResult::InvalidChannel;

    const uint8_t idx = relayIndex(channel);

    // Skip update when value unchanged — no dirty flag, no version bump, no
    // timestamp update.  Prevents spurious sync cycles on repeated writes.
    if (m_relayStates[idx] == state) return DeviceStateResult::Success;

    m_relayStates[idx]    = state;
    m_relayDirty          = true;
    m_lastRelayUpdateMs   = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

RelayState DeviceStateManager::getRelayState(RelayChannel channel) const
{
    // Return a safe default rather than crash or emit an error code — getters
    // must never block or abort embedded code paths.
    if (!isValidRelayChannel(channel)) return RelayState::Off;
    return m_relayStates[relayIndex(channel)];
}

// =============================================================================
// Switch state
// =============================================================================

DeviceStateResult DeviceStateManager::setSwitchState(SwitchChannel channel, SwitchState state)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (!isValidSwitchChannel(channel)) return DeviceStateResult::InvalidChannel;

    const uint8_t idx = switchIndex(channel);

    if (m_switchStates[idx] == state) return DeviceStateResult::Success;

    m_switchStates[idx]   = state;
    m_switchDirty         = true;
    m_lastSwitchUpdateMs  = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

SwitchState DeviceStateManager::getSwitchState(SwitchChannel channel) const
{
    if (!isValidSwitchChannel(channel)) return SwitchState::Released;
    return m_switchStates[switchIndex(channel)];
}

// =============================================================================
// WiFi state
// =============================================================================

DeviceStateResult DeviceStateManager::setWiFiState(WiFiState state)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (m_wifiState == state) return DeviceStateResult::Success;

    m_wifiState          = state;
    m_wifiDirty          = true;
    m_lastWiFiUpdateMs   = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

WiFiState DeviceStateManager::getWiFiState() const
{
    return m_wifiState;
}

// =============================================================================
// Firebase state
// =============================================================================

DeviceStateResult DeviceStateManager::setFirebaseState(FirebaseState state)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (m_firebaseState == state) return DeviceStateResult::Success;

    m_firebaseState          = state;
    m_firebaseDirty          = true;
    m_lastFirebaseUpdateMs   = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

FirebaseState DeviceStateManager::getFirebaseState() const
{
    return m_firebaseState;
}

// =============================================================================
// Device mode
// =============================================================================

DeviceStateResult DeviceStateManager::setDeviceMode(DeviceMode mode)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (m_deviceMode == mode) return DeviceStateResult::Success;

    m_deviceMode         = mode;
    m_systemDirty        = true;
    m_lastSystemUpdateMs = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

DeviceMode DeviceStateManager::getDeviceMode() const
{
    return m_deviceMode;
}

// =============================================================================
// Online status
// =============================================================================

DeviceStateResult DeviceStateManager::setOnline(bool online)
{
    DeviceStateResult check = checkReady();
    if (check != DeviceStateResult::Success) return check;

    if (m_online == online) return DeviceStateResult::Success;

    m_online             = online;
    m_systemDirty        = true;
    m_lastSystemUpdateMs = millis();
    incrementVersion();

    return DeviceStateResult::Success;
}

bool DeviceStateManager::isOnline() const
{
    return m_online;
}

// =============================================================================
// System
// =============================================================================

uint32_t DeviceStateManager::getUptime() const
{
    if (!m_initialized) return 0u;
    // millis() wraps at ~49.7 days; so does this subtraction — intentional.
    return millis() - m_uptimeStartMs;
}

DeviceStateResult DeviceStateManager::reset()
{
    // Bypass the enabled check: callers must be able to reset state even
    // during error recovery when the manager has been disabled.
    if (!m_initialized) return DeviceStateResult::NotInitialized;

    initState();
    m_uptimeStartMs = millis();

    return DeviceStateResult::Success;
}

// =============================================================================
// Dirty flags — query
// =============================================================================

bool DeviceStateManager::isRelayDirty()    const { return m_relayDirty;    }
bool DeviceStateManager::isSwitchDirty()   const { return m_switchDirty;   }
bool DeviceStateManager::isWiFiDirty()     const { return m_wifiDirty;     }
bool DeviceStateManager::isFirebaseDirty() const { return m_firebaseDirty; }
bool DeviceStateManager::isSystemDirty()   const { return m_systemDirty;   }

// =============================================================================
// Dirty flags — clear
// =============================================================================

void DeviceStateManager::clearRelayDirty()    { m_relayDirty    = false; }
void DeviceStateManager::clearSwitchDirty()   { m_switchDirty   = false; }
void DeviceStateManager::clearWiFiDirty()     { m_wifiDirty     = false; }
void DeviceStateManager::clearFirebaseDirty() { m_firebaseDirty = false; }
void DeviceStateManager::clearSystemDirty()   { m_systemDirty   = false; }

void DeviceStateManager::clearAllDirty()
{
    m_relayDirty    = false;
    m_switchDirty   = false;
    m_wifiDirty     = false;
    m_firebaseDirty = false;
    m_systemDirty   = false;
}

// =============================================================================
// State version
// =============================================================================

uint32_t DeviceStateManager::getVersion() const
{
    return m_version;
}

// =============================================================================
// Last update timestamps
// =============================================================================

uint32_t DeviceStateManager::getLastRelayUpdate()    const { return m_lastRelayUpdateMs;    }
uint32_t DeviceStateManager::getLastSwitchUpdate()   const { return m_lastSwitchUpdateMs;   }
uint32_t DeviceStateManager::getLastWiFiUpdate()     const { return m_lastWiFiUpdateMs;     }
uint32_t DeviceStateManager::getLastFirebaseUpdate() const { return m_lastFirebaseUpdateMs; }

// =============================================================================
// Private helpers
// =============================================================================

DeviceStateResult DeviceStateManager::checkReady() const
{
    if (!m_initialized) return DeviceStateResult::NotInitialized;
    if (!m_enabled)     return DeviceStateResult::Disabled;
    return DeviceStateResult::Success;
}

bool DeviceStateManager::isValidRelayChannel(RelayChannel channel)
{
    // enum class RelayChannel values equal their array indices (0–15); any
    // value outside [0, MAX_RELAYS) is either corrupted or a future extension.
    return static_cast<uint8_t>(channel) < MAX_RELAYS;
}

bool DeviceStateManager::isValidSwitchChannel(SwitchChannel channel)
{
    return static_cast<uint8_t>(channel) < MAX_SWITCHES;
}

uint8_t DeviceStateManager::relayIndex(RelayChannel channel)
{
    // RelayChannel enumerators equal their zero-based indices by design, so
    // this cast is always safe after isValidRelayChannel() passes.
    return static_cast<uint8_t>(channel);
}

uint8_t DeviceStateManager::switchIndex(SwitchChannel channel)
{
    return static_cast<uint8_t>(channel);
}

void DeviceStateManager::incrementVersion()
{
    // Intentional wrap: callers must track snapshot inequality, not absolute
    // ordering.  A wrap at UINT32_MAX never implies "no change".
    ++m_version;
}

void DeviceStateManager::initState()
{
    // Shared reset path for both begin() and reset().  Sets every runtime
    // value to its safe power-on default so the object is internally
    // consistent at any point it is queried.

    for (uint8_t i = 0u; i < MAX_RELAYS; ++i) {
        m_relayStates[i] = RelayState::Off;
    }
    for (uint8_t i = 0u; i < MAX_SWITCHES; ++i) {
        m_switchStates[i] = SwitchState::Released;
    }

    m_wifiState     = WiFiState::Idle;
    m_firebaseState = FirebaseState::NotConfigured;
    m_deviceMode    = DeviceMode::Booting;
    m_online        = false;

    m_relayDirty    = false;
    m_switchDirty   = false;
    m_wifiDirty     = false;
    m_firebaseDirty = false;
    m_systemDirty   = false;

    m_version = 0u;

    m_lastRelayUpdateMs    = 0u;
    m_lastSwitchUpdateMs   = 0u;
    m_lastWiFiUpdateMs     = 0u;
    m_lastFirebaseUpdateMs = 0u;
    m_lastSystemUpdateMs   = 0u;
}
