#pragma once

// DeviceStateManager — the single source of truth for all runtime device state.
//
// Stores the live state of every device component while the system is running.
// All data lives in RAM only; nothing is read from or written to flash.  On
// reboot, state is lost.  For persistent values, use PreferencesManager.
//
// DeviceStateManager is a passive in-memory database.  It never:
//   - controls relay GPIO
//   - connects to WiFi
//   - communicates with Firebase
//   - performs synchronization
//   - writes to flash storage
//
// Every manager writes only its own state.  Every manager may read any state.
// The application coordinates managers; DeviceStateManager never calls them.
//
// Typical usage:
//   DeviceStateManager deviceState;
//   void setup() { deviceState.begin(); }
//   void loop()  { deviceState.loop();  }
//
// Duplicate-write protection: setters compare the new value with the current
// value before writing.  Identical values produce no dirty flag change and no
// version increment.

#include <Arduino.h>
#include <stdint.h>

// Include the other managers' headers only for their shared enum types.
// These are compile-time type dependencies, not runtime communication.
#include "../../Device/RelayManager/RelayManager.h"
#include "../../Network/IoTWiFiManager/IoTWiFiManager.h"
#include "../../Cloud/FirebaseManager/FirebaseManager.h"

// ---------------------------------------------------------------------------
// SwitchChannel
//
// Type-safe switch identifier used by SwitchManager (future) and the
// application to report switch activity through DeviceStateManager.
// Values equal their zero-based indices so channelToIndex() is a plain cast.
// ---------------------------------------------------------------------------
enum class SwitchChannel : uint8_t
{
    Switch1  =  0,
    Switch2  =  1,
    Switch3  =  2,
    Switch4  =  3,
    Switch5  =  4,
    Switch6  =  5,
    Switch7  =  6,
    Switch8  =  7,
    Switch9  =  8,
    Switch10 =  9,
    Switch11 = 10,
    Switch12 = 11,
    Switch13 = 12,
    Switch14 = 13,
    Switch15 = 14,
    Switch16 = 15
};

// ---------------------------------------------------------------------------
// SwitchState
// ---------------------------------------------------------------------------
enum class SwitchState : uint8_t
{
    Released = 0,  // default on zero-init; switch idle
    Pressed  = 1
};

// ---------------------------------------------------------------------------
// DeviceMode
// ---------------------------------------------------------------------------
enum class DeviceMode : uint8_t
{
    Booting,      // power-on initialization phase
    Normal,       // fully operational
    Maintenance,  // restricted operation; OTA or diagnostics active
    SafeMode,     // minimal feature set; hardware or config fault detected
    Updating      // firmware update in progress
};

// ---------------------------------------------------------------------------
// DeviceStateResult
//
// Returned by all public setters and lifecycle operations.
// Getters return safe defaults rather than error codes.
// ---------------------------------------------------------------------------
enum class DeviceStateResult : uint8_t
{
    Success,        ///< Operation completed (including a no-op duplicate write).
    InvalidChannel, ///< Channel index is out of the supported range.
    InvalidState,   ///< State value is not a recognised enum value.
    Disabled,       ///< Manager is disabled; operation rejected.
    NotInitialized  ///< begin() has not been called yet.
};

// ---------------------------------------------------------------------------
// DeviceStateManager
// ---------------------------------------------------------------------------

class DeviceStateManager
{
public:

    /// Maximum number of relay state slots (matches RelayManager::MAX_RELAYS).
    static constexpr uint8_t MAX_RELAYS   = 16u;

    /// Maximum number of switch state slots.
    static constexpr uint8_t MAX_SWITCHES = 16u;

    // -----------------------------------------------------------------------
    // Construction
    //
    // Only initialises internal members to safe defaults.
    // No hardware access, no millis() call, no side effects.
    // -----------------------------------------------------------------------
    DeviceStateManager();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises runtime state to defaults and records the boot time.
     *
     * Must be called once from setup() before any setter is used.
     * Calling begin() a second time resets all runtime state and restarts the
     * uptime counter (useful for runtime re-initialisation).
     */
    void begin();

    /**
     * @brief Periodic update hook — call from the main loop().
     *
     * Currently a no-op.  Reserved for future background work such as
     * watchdog updates and timed state expiry.
     */
    void loop();

    /**
     * @brief Re-enables the manager after a disable() call.
     *
     * Returns immediately if the manager is not in the Disabled state.
     */
    void enable();

    /**
     * @brief Suspends all setter operations.
     *
     * Every setter returns DeviceStateResult::Disabled until enable() is
     * called.  Getters, dirty flags, and version queries remain accessible.
     */
    void disable();

    /**
     * @brief Returns whether the manager currently accepts setter calls.
     *
     * @return true when enabled, false when disabled.
     */
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Relay state
    // -----------------------------------------------------------------------

    /**
     * @brief Records the current logical state of a relay channel.
     *
     * Skips the update when the new value matches the stored value so that
     * only genuine state transitions mark the category dirty and advance the
     * version counter.
     *
     * @param channel Relay channel (Relay1–Relay16).
     * @param state   New relay state.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setRelayState(RelayChannel channel, RelayState state);

    /**
     * @brief Returns the latest recorded state for a relay channel.
     *
     * Returns RelayState::Off for an invalid or uninitialised channel; callers
     * never receive an error code since getters are always safe to call.
     *
     * @param channel Relay channel to query.
     * @return Stored RelayState, or Off if the channel is invalid.
     */
    RelayState getRelayState(RelayChannel channel) const;

    // -----------------------------------------------------------------------
    // Switch state
    // -----------------------------------------------------------------------

    /**
     * @brief Records the current state of a switch channel.
     *
     * @param channel Switch channel (Switch1–Switch16).
     * @param state   New switch state.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setSwitchState(SwitchChannel channel, SwitchState state);

    /**
     * @brief Returns the latest recorded state for a switch channel.
     *
     * Returns SwitchState::Released for an invalid channel.
     *
     * @param channel Switch channel to query.
     * @return Stored SwitchState, or Released if the channel is invalid.
     */
    SwitchState getSwitchState(SwitchChannel channel) const;

    // -----------------------------------------------------------------------
    // WiFi state
    // -----------------------------------------------------------------------

    /**
     * @brief Records the current WiFi connection state.
     *
     * @param state New WiFiState value reported by IoTWiFiManager.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setWiFiState(WiFiState state);

    /**
     * @brief Returns the latest recorded WiFi state.
     *
     * Returns WiFiState::Idle if begin() has not been called.
     *
     * @return Current WiFiState.
     */
    WiFiState getWiFiState() const;

    // -----------------------------------------------------------------------
    // Firebase state
    // -----------------------------------------------------------------------

    /**
     * @brief Records the current Firebase connection state.
     *
     * @param state New FirebaseState value reported by FirebaseManager.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setFirebaseState(FirebaseState state);

    /**
     * @brief Returns the latest recorded Firebase state.
     *
     * Returns FirebaseState::NotConfigured if begin() has not been called.
     *
     * @return Current FirebaseState.
     */
    FirebaseState getFirebaseState() const;

    // -----------------------------------------------------------------------
    // Device mode
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the current operating mode of the device.
     *
     * @param mode New DeviceMode value.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setDeviceMode(DeviceMode mode);

    /**
     * @brief Returns the current device operating mode.
     *
     * Returns DeviceMode::Booting if begin() has not been called.
     *
     * @return Current DeviceMode.
     */
    DeviceMode getDeviceMode() const;

    // -----------------------------------------------------------------------
    // Online status
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the composite online flag.
     *
     * Typically set to true when both WiFi is Connected and Firebase is Ready,
     * and false when either drops.  The application (or a future SyncManager)
     * is responsible for deciding when the device is "online".
     *
     * @param online true when the device is considered online.
     * @return DeviceStateResult indicating success or the rejection reason.
     */
    DeviceStateResult setOnline(bool online);

    /**
     * @brief Returns the composite online flag.
     *
     * @return true when the application has signalled online status.
     */
    bool isOnline() const;

    // -----------------------------------------------------------------------
    // System
    // -----------------------------------------------------------------------

    /**
     * @brief Returns elapsed milliseconds since begin() was last called.
     *
     * Uses millis() internally; wraps at ~49.7 days like millis() itself.
     * Returns 0 if begin() has not been called.
     *
     * @return Uptime in milliseconds.
     */
    uint32_t getUptime() const;

    /**
     * @brief Resets all runtime state to post-begin() defaults.
     *
     * Clears relay states, switch states, network states, dirty flags, and
     * the version counter.  Restarts the uptime timer.  Does NOT touch flash
     * storage — PreferencesManager owns persistent data.
     *
     * Bypass the enabled check so callers can reset state during error
     * recovery even when the manager is disabled.
     *
     * @return Success, or NotInitialized if begin() was never called.
     */
    DeviceStateResult reset();

    // -----------------------------------------------------------------------
    // Dirty flags
    //
    // A category is "dirty" when at least one value in that category has
    // changed since the dirty flag was last cleared.  Future SyncManager and
    // EventBus implementations poll these flags to detect work to do.
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true when any relay state has changed since the last clearRelayDirty() call.
     * @return Relay dirty flag.
     */
    bool isRelayDirty()    const;

    /**
     * @brief Returns true when any switch state has changed since the last clearSwitchDirty() call.
     * @return Switch dirty flag.
     */
    bool isSwitchDirty()   const;

    /**
     * @brief Returns true when the WiFi state has changed since the last clearWiFiDirty() call.
     * @return WiFi dirty flag.
     */
    bool isWiFiDirty()     const;

    /**
     * @brief Returns true when the Firebase state has changed since the last clearFirebaseDirty() call.
     * @return Firebase dirty flag.
     */
    bool isFirebaseDirty() const;

    /**
     * @brief Returns true when any system state (mode, online) has changed since the last clearSystemDirty() call.
     * @return System dirty flag.
     */
    bool isSystemDirty()   const;

    /**
     * @brief Clears the relay dirty flag.
     *
     * Normally called by SyncManager or EventBus after successful processing.
     */
    void clearRelayDirty();

    /**
     * @brief Clears the switch dirty flag.
     */
    void clearSwitchDirty();

    /**
     * @brief Clears the WiFi dirty flag.
     */
    void clearWiFiDirty();

    /**
     * @brief Clears the Firebase dirty flag.
     */
    void clearFirebaseDirty();

    /**
     * @brief Clears the system dirty flag.
     */
    void clearSystemDirty();

    /**
     * @brief Clears all category dirty flags in one call.
     *
     * Convenience method for callers that process all categories at once.
     */
    void clearAllDirty();

    // -----------------------------------------------------------------------
    // State version
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the global runtime state version counter.
     *
     * Incremented on every successful setter call that results in a genuine
     * value change.  Allows snapshot comparisons without inspecting individual
     * categories.  Wraps from UINT32_MAX back to 0 — callers should compare
     * snapshots with inequality, not greater-than.
     *
     * @return Current version counter value.
     */
    uint32_t getVersion() const;

    // -----------------------------------------------------------------------
    // Last update timestamps
    //
    // Return the millis() value recorded at the last genuine state change in
    // each category.  Useful for timeout detection and synchronisation logic.
    // Returns 0 if the category has not changed since begin().
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the millis() value of the last relay state change.
     * @return Timestamp in milliseconds, or 0 if no relay change has occurred.
     */
    uint32_t getLastRelayUpdate()    const;

    /**
     * @brief Returns the millis() value of the last switch state change.
     * @return Timestamp in milliseconds, or 0 if no switch change has occurred.
     */
    uint32_t getLastSwitchUpdate()   const;

    /**
     * @brief Returns the millis() value of the last WiFi state change.
     * @return Timestamp in milliseconds, or 0 if WiFi state has not changed.
     */
    uint32_t getLastWiFiUpdate()     const;

    /**
     * @brief Returns the millis() value of the last Firebase state change.
     * @return Timestamp in milliseconds, or 0 if Firebase state has not changed.
     */
    uint32_t getLastFirebaseUpdate() const;

private:

    // -----------------------------------------------------------------------
    // Lifecycle state
    // -----------------------------------------------------------------------
    bool     m_initialized;  ///< true after begin() has been called.
    bool     m_enabled;      ///< Application-level gate for setters.
    uint32_t m_uptimeStartMs; ///< millis() captured in begin() / reset().

    // -----------------------------------------------------------------------
    // Hardware runtime state
    // -----------------------------------------------------------------------
    RelayState  m_relayStates[MAX_RELAYS];   ///< Indexed by RelayChannel value.
    SwitchState m_switchStates[MAX_SWITCHES]; ///< Indexed by SwitchChannel value.

    // -----------------------------------------------------------------------
    // Network runtime state
    // -----------------------------------------------------------------------
    WiFiState     m_wifiState;
    FirebaseState m_firebaseState;

    // -----------------------------------------------------------------------
    // Device runtime state
    // -----------------------------------------------------------------------
    DeviceMode m_deviceMode;
    bool       m_online;

    // -----------------------------------------------------------------------
    // Dirty flags — one per state category
    // -----------------------------------------------------------------------
    bool m_relayDirty;
    bool m_switchDirty;
    bool m_wifiDirty;
    bool m_firebaseDirty;
    bool m_systemDirty;

    // -----------------------------------------------------------------------
    // Global version counter
    // -----------------------------------------------------------------------
    uint32_t m_version;

    // -----------------------------------------------------------------------
    // Per-category last-change timestamps
    // -----------------------------------------------------------------------
    uint32_t m_lastRelayUpdateMs;
    uint32_t m_lastSwitchUpdateMs;
    uint32_t m_lastWiFiUpdateMs;
    uint32_t m_lastFirebaseUpdateMs;
    uint32_t m_lastSystemUpdateMs;

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Checks that the manager is initialised and enabled.
     * @return Success if ready, otherwise the appropriate error code.
     */
    DeviceStateResult checkReady() const;

    /**
     * @brief Validates a relay channel index.
     * @param channel Channel enum to test.
     * @return true if the channel maps to a valid array index.
     */
    static bool isValidRelayChannel(RelayChannel channel);

    /**
     * @brief Validates a switch channel index.
     * @param channel Channel enum to test.
     * @return true if the channel maps to a valid array index.
     */
    static bool isValidSwitchChannel(SwitchChannel channel);

    /// Converts RelayChannel to its zero-based array index (plain cast — enum values equal indices).
    static uint8_t relayIndex(RelayChannel channel);

    /// Converts SwitchChannel to its zero-based array index (plain cast — enum values equal indices).
    static uint8_t switchIndex(SwitchChannel channel);

    /// Increments the global version counter; wrap-around is intentional.
    void incrementVersion();

    /// Sets all runtime state to safe post-begin() defaults.
    /// Shared between begin() and reset() to avoid code duplication.
    void initState();
};
