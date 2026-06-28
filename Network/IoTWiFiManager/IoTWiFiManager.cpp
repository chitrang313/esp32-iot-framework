#include "IoTWiFiManager.h"

#include <stdarg.h>

// ---------------------------------------------------------------------------
// State transition table (for maintainer reference)
//
//   Idle          --[ begin() ]------------------> Connecting
//   Connecting    --[ GOT_IP event ]-----------  -> Connected
//   Connecting    --[ timeout ]------------------> Reconnecting
//   Connected     --[ DISCONNECTED event ]-------> Reconnecting
//   Reconnecting  --[ backoff elapsed ]----------> Connecting
//   Reconnecting  --[ reconnect() ]--------------> Connecting   (resets backoff)
//   Any           --[ disconnect() ]-------------> Disconnected
//   Disconnected  --[ reconnect() ]--------------> Connecting   (resets backoff)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

IoTWiFiManager::IoTWiFiManager(const char* ssid, const char* password)
    : m_ssid(ssid)
    , m_password(password)
    , m_state(WiFiState::Idle)
    , m_connectStartMs(0)
    , m_reconnectAfterMs(0)
    , m_connectedSinceMs(0)
    , m_reconnectCount(0)
    , m_lastDisconnectReason(0)
    , m_eventConnected(false)
    , m_eventDisconnected(false)
    , m_eventDisconnectReason(0)
    , m_retryAttempt(0)
    , m_reconnectPending(false)
{
}

// ---------------------------------------------------------------------------
// Public lifecycle
// ---------------------------------------------------------------------------

void IoTWiFiManager::begin()
{
    log("Initialising");

    WiFi.mode(WIFI_STA);

    // ESP32 Arduino core 3.x removed the userData parameter from WiFi.onEvent().
    // The capturing lambda provides 'this' to the instance callback instead.
    WiFi.onEvent(
        [this](WiFiEvent_t event, WiFiEventInfo_t info) {
            this->onWiFiEvent(event, info);
        }
    );

    connect();
}

void IoTWiFiManager::loop()
{
    updateState();
}

void IoTWiFiManager::reconnect()
{
    log("Manual reconnect requested");
    resetReconnectState();
    connect();
}

void IoTWiFiManager::disconnect()
{
    log("Manual disconnect requested — auto-reconnect disabled");
    disconnectInternal();
    transitionTo(WiFiState::Disconnected);
}

// ---------------------------------------------------------------------------
// Public query API
// ---------------------------------------------------------------------------

bool IoTWiFiManager::isConnected() const
{
    return m_state == WiFiState::Connected;
}

bool IoTWiFiManager::isConnecting() const
{
    return m_state == WiFiState::Connecting;
}

WiFiState IoTWiFiManager::getState() const
{
    return m_state;
}

String IoTWiFiManager::getIPAddress() const
{
    if (m_state != WiFiState::Connected)
    {
        return String("0.0.0.0");
    }
    return WiFi.localIP().toString();
}

String IoTWiFiManager::getSSID() const
{
    if (m_state != WiFiState::Connected)
    {
        return String();
    }
    return WiFi.SSID();
}

int32_t IoTWiFiManager::getRSSI() const
{
    if (m_state != WiFiState::Connected)
    {
        return 0;
    }
    return WiFi.RSSI();
}

uint32_t IoTWiFiManager::getReconnectCount() const
{
    return m_reconnectCount;
}

uint32_t IoTWiFiManager::getConnectedDuration() const
{
    if (m_state != WiFiState::Connected)
    {
        return 0;
    }
    return (millis() - m_connectedSinceMs) / 1000UL;
}

int IoTWiFiManager::getLastDisconnectReason() const
{
    return m_lastDisconnectReason;
}

// ---------------------------------------------------------------------------
// Private — connection helpers
// ---------------------------------------------------------------------------

void IoTWiFiManager::connect()
{
    logf("Connecting to SSID: %s (attempt %u)", m_ssid, m_retryAttempt + 1);

    WiFi.disconnect(true); // ensure clean slate before each attempt
    WiFi.begin(m_ssid, m_password);

    m_connectStartMs    = millis();
    m_reconnectPending  = false;

    transitionTo(WiFiState::Connecting);
}

void IoTWiFiManager::disconnectInternal()
{
    WiFi.disconnect(true);
}

void IoTWiFiManager::scheduleReconnect()
{
    // Exponential backoff with a hard ceiling.
    // Attempt 0 -> 2 s, 1 -> 4 s, 2 -> 8 s, 3 -> 16 s, 4+ -> 30 s cap,
    // then at attempt 5+ the ceiling below clamps to 60 s.
    const uint32_t delayMs = min(
        RECONNECT_BASE_DELAY_MS << m_retryAttempt,   // 2^n * base
        RECONNECT_MAX_DELAY_MS
    );

    m_reconnectAfterMs = millis() + delayMs;
    m_reconnectPending = true;
    m_retryAttempt++;

    logf("Retry %u scheduled in %u ms", m_retryAttempt, delayMs);

    transitionTo(WiFiState::Reconnecting);
}

void IoTWiFiManager::resetReconnectState()
{
    m_retryAttempt     = 0;
    m_reconnectPending = false;
}

// ---------------------------------------------------------------------------
// Private — state machine driver (called every loop() tick)
// ---------------------------------------------------------------------------

void IoTWiFiManager::updateState()
{
    // --- Consume events set by the WiFi callback -------------------------
    // Read volatile flags exactly once into local copies to avoid races.
    const bool gotConnected   = m_eventConnected;
    const bool gotDisconnected = m_eventDisconnected;
    const int  disconnectReason = m_eventDisconnectReason;

    if (gotConnected)
    {
        m_eventConnected = false;

        if (m_state == WiFiState::Connecting)
        {
            resetReconnectState();
            m_connectedSinceMs = millis();

            logf("Connected — IP: %s  RSSI: %d dBm",
                 WiFi.localIP().toString().c_str(),
                 WiFi.RSSI());

            transitionTo(WiFiState::Connected);
        }
    }

    if (gotDisconnected)
    {
        m_eventDisconnected    = false;
        m_lastDisconnectReason = disconnectReason;

        // WiFi.disconnect(true) inside connect() fires a spurious DISCONNECTED
        // event (reason 8 / ASSOC_LEAVE) as a side-effect of clearing the
        // interface before each attempt.  A genuine disconnect from a live AP
        // cannot arrive within 500 ms of a fresh connection attempt, so events
        // in that window are discarded.
        const bool isSpuriousFromConnect = (millis() - m_connectStartMs) < 500UL;

        if (!isSpuriousFromConnect
            && (m_state == WiFiState::Connected || m_state == WiFiState::Connecting))
        {
            logf("Disconnected — reason: %d", m_lastDisconnectReason);

            // Do NOT call WiFi.begin() here; schedule it for the next window.
            scheduleReconnect();
        }
        // If already Reconnecting or Disconnected, ignore duplicate events.
    }

    // --- State-specific periodic logic -----------------------------------

    switch (m_state)
    {
        case WiFiState::Connecting:
        {
            const uint32_t elapsed = millis() - m_connectStartMs;

            if (elapsed >= CONNECT_TIMEOUT_MS)
            {
                log("Connect attempt timed out");
                scheduleReconnect();
            }
            break;
        }

        case WiFiState::Reconnecting:
        {
            if (m_reconnectPending && millis() >= m_reconnectAfterMs)
            {
                m_reconnectCount++;
                connect();
            }
            break;
        }

        case WiFiState::Connected:
        {
            // Periodic RSSI log every 30 seconds — useful for long-running diagnostics.
            static uint32_t lastRssiLogMs = 0;
            if (millis() - lastRssiLogMs >= 30000UL)
            {
                lastRssiLogMs = millis();
                logf("RSSI: %d dBm  uptime: %u s",
                     WiFi.RSSI(),
                     getConnectedDuration());
            }
            break;
        }

        case WiFiState::Idle:
        case WiFiState::Disconnected:
            // Nothing to drive; waiting for explicit reconnect() call.
            break;
    }
}

// ---------------------------------------------------------------------------
// Private — explicit state transition
// ---------------------------------------------------------------------------

void IoTWiFiManager::transitionTo(WiFiState nextState)
{
    if (m_state == nextState)
    {
        return; // no-op — guard against redundant transitions
    }

    m_state = nextState;
}

// ---------------------------------------------------------------------------
// WiFi event callback — runs in the WiFi/LwIP task context.
// IMPORTANT: must ONLY set flags and record data.  No WiFi calls, no logging,
// no heavy work.  loop() / updateState() consumes these flags safely.
// ---------------------------------------------------------------------------

void IoTWiFiManager::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event)
    {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            m_eventConnected    = true;
            m_eventDisconnected = false;
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            m_eventDisconnected     = true;
            m_eventConnected        = false;
            m_eventDisconnectReason = static_cast<int>(info.wifi_sta_disconnected.reason);
            break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

void IoTWiFiManager::log(const char* message) const
{
#ifndef IOT_WIFI_LOG_DISABLE
    Serial.print("[WiFi] ");
    Serial.println(message);
#endif
}

void IoTWiFiManager::logf(const char* format, ...) const
{
#ifndef IOT_WIFI_LOG_DISABLE
    char buf[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.print("[WiFi] ");
    Serial.println(buf);
#endif
}
