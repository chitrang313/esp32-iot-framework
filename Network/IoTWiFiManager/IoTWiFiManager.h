#pragma once

#include <Arduino.h>
#include <WiFi.h>

// ---------------------------------------------------------------------------
// WiFiState — all legal states the manager can occupy.
// Transitions are documented in IoTWiFiManager.cpp (see transitionTo()).
// ---------------------------------------------------------------------------
enum class WiFiState
{
    Idle,          // begin() not yet called
    Connecting,    // WiFi.begin() issued, waiting for association
    Connected,     // IP assigned, link up
    Reconnecting,  // waiting for backoff delay before next attempt
    Disconnected   // explicit disconnect() call — will NOT auto-reconnect
};

// ---------------------------------------------------------------------------
// IoTWiFiManager
//
// Owns the full WiFi lifecycle for an ESP32 device that must stay online 24/7.
// Callers never touch WiFi.begin() directly; they only query this manager.
//
// Typical usage:
//   IoTWiFiManager wifi("MySSID", "MyPassword");
//   void setup() { wifi.begin(); }
//   void loop()  { wifi.loop();  }
// ---------------------------------------------------------------------------
class IoTWiFiManager
{
public:
    // Configurable timing constants — tweak per project without touching logic.
    static constexpr uint32_t CONNECT_TIMEOUT_MS      = 15000UL; // 15 s before giving up on a connect attempt
    static constexpr uint32_t RECONNECT_BASE_DELAY_MS =  2000UL; // first backoff window
    static constexpr uint32_t RECONNECT_MAX_DELAY_MS  = 60000UL; // ceiling for backoff

    // -----------------------------------------------------------------------
    // Constructor — credentials stored by pointer; caller must keep them alive
    // for the lifetime of this object (typically global const char* literals).
    // -----------------------------------------------------------------------
    IoTWiFiManager(const char* ssid, const char* password);

    // -----------------------------------------------------------------------
    // Public lifecycle API
    // -----------------------------------------------------------------------

    // Call once from setup(). Registers WiFi event handler and issues first
    // connection attempt.
    void begin();

    // Call every iteration of loop(). Drives the state machine; never blocks.
    void loop();

    // Force an immediate reconnect attempt (resets backoff counter).
    // Safe to call from any state.
    void reconnect();

    // Disconnect and stay disconnected. Auto-reconnect will NOT resume.
    // Call reconnect() to re-enable automatic management.
    void disconnect();

    // -----------------------------------------------------------------------
    // Public query API — all const, safe to call at any time
    // -----------------------------------------------------------------------

    bool         isConnected()           const;
    bool         isConnecting()          const;

    WiFiState    getState()              const;

    // Returns dotted-decimal IP string, or "0.0.0.0" when not connected.
    String       getIPAddress()          const;

    // Returns associated SSID, or empty string when not connected.
    String       getSSID()               const;

    // Returns current RSSI in dBm, or 0 when not connected.
    int32_t      getRSSI()               const;

    // Total number of reconnect attempts since begin() was called.
    uint32_t     getReconnectCount()     const;

    // Seconds of continuous connectivity from last successful association.
    // Resets to 0 on each disconnect.
    uint32_t     getConnectedDuration()  const;

    // ESP32 WiFi disconnect reason code from the last DISCONNECTED event.
    // 0 means no disconnect has been recorded yet.
    int          getLastDisconnectReason() const;

private:
    // -----------------------------------------------------------------------
    // Stored credentials
    // -----------------------------------------------------------------------
    const char* m_ssid;
    const char* m_password;

    // -----------------------------------------------------------------------
    // State machine
    // -----------------------------------------------------------------------
    WiFiState m_state;

    // Performs the transition, logs it, and updates bookkeeping timestamps.
    void transitionTo(WiFiState nextState);

    // -----------------------------------------------------------------------
    // Connection helpers — called only from loop() or begin(), never from
    // the WiFi event callback.
    // -----------------------------------------------------------------------
    void connect();
    void disconnectInternal();
    void scheduleReconnect();
    void resetReconnectState();

    // Called every loop() tick to drive timeout and backoff logic.
    void updateState();

    // -----------------------------------------------------------------------
    // Timing
    // -----------------------------------------------------------------------
    uint32_t m_connectStartMs;    // millis() when last connect attempt began
    uint32_t m_reconnectAfterMs;  // millis() when next reconnect attempt may fire
    uint32_t m_connectedSinceMs;  // millis() when current connected period began

    // -----------------------------------------------------------------------
    // Diagnostics / telemetry
    // -----------------------------------------------------------------------
    uint32_t m_reconnectCount;
    int      m_lastDisconnectReason;

    // -----------------------------------------------------------------------
    // Flags set by the WiFi event callback — consumed by loop() / updateState()
    // The callback must do nothing more than flip these flags and record data.
    // -----------------------------------------------------------------------
    volatile bool m_eventConnected;     // WIFI_EVENT_STA_GOT_IP fired
    volatile bool m_eventDisconnected;  // WIFI_EVENT_STA_DISCONNECTED fired
    volatile int  m_eventDisconnectReason;

    // -----------------------------------------------------------------------
    // Reconnect backoff state
    // -----------------------------------------------------------------------
    uint8_t  m_retryAttempt;   // increments with each failed attempt
    bool     m_reconnectPending; // true while waiting for backoff window

    // -----------------------------------------------------------------------
    // Instance event callback invoked via a capturing lambda registered in
    // begin().  ESP32 Arduino core 3.x removed the userData dispatch
    // parameter; the lambda capture provides 'this' instead.
    // -----------------------------------------------------------------------
    void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    // -----------------------------------------------------------------------
    // Logging helpers — wrap Serial.printf so logging can be disabled by
    // defining IOT_WIFI_LOG_DISABLE before including this header.
    // -----------------------------------------------------------------------
    void log(const char* message)  const;
    void logf(const char* format, ...) const;
};
