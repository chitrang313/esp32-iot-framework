#include "LoggerManager.h"
#include <stdio.h>   // snprintf

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

LoggerManager::LoggerManager()
    : m_initialized     (false)
    , m_enabled         (true)
    , m_timestampEnabled(true)
    , m_logLevel        (LogLevel::Debug)
    , m_eventBus        (nullptr)
    , m_outputCallback  (nullptr)
    , m_logCount        (0u)
{
    m_buffer[0] = '\0';
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void LoggerManager::begin(EventBus& eventBus)
{
    m_eventBus        = &eventBus;
    m_logCount        = 0u;
    m_outputCallback  = nullptr;
    m_initialized     = true;
    m_enabled         = true;

    // Subscribe after marking m_initialized = true so that any event arriving
    // in the same loop() tick immediately passes the initialized check.
    subscribeToEvents();
}

void LoggerManager::loop()
{
    // Phase 1: Serial is written synchronously inside the EventBus callback.
    // This method is reserved for future output queuing (SD, MQTT, Firebase).
}

void LoggerManager::enable()
{
    m_enabled = true;
}

void LoggerManager::disable()
{
    m_enabled = false;
}

bool LoggerManager::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

LoggerResult LoggerManager::debug(const char* message)
{
    return log(LogLevel::Debug, message);
}

LoggerResult LoggerManager::info(const char* message)
{
    return log(LogLevel::Info, message);
}

LoggerResult LoggerManager::warning(const char* message)
{
    return log(LogLevel::Warning, message);
}

LoggerResult LoggerManager::error(const char* message)
{
    return log(LogLevel::Error, message);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

LoggerResult LoggerManager::setLogLevel(LogLevel level)
{
    // LogLevel is an enum class but callers may cast arbitrary integers.
    // Reject anything above LogLevel::None (the highest valid value).
    if (static_cast<uint8_t>(level) > static_cast<uint8_t>(LogLevel::None))
    {
        return LoggerResult::InvalidLevel;
    }
    m_logLevel = level;
    return LoggerResult::Success;
}

LogLevel LoggerManager::getLogLevel() const
{
    return m_logLevel;
}

void LoggerManager::enableTimestamp()
{
    m_timestampEnabled = true;
}

void LoggerManager::disableTimestamp()
{
    m_timestampEnabled = false;
}

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

void LoggerManager::setOutputCallback(LogOutputCallback callback)
{
    // nullptr is valid: reverts to Serial output.
    m_outputCallback = callback;
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

uint32_t LoggerManager::getLogCount() const
{
    return m_logCount;
}

void LoggerManager::clearLogCount()
{
    m_logCount = 0u;
}

// ---------------------------------------------------------------------------
// Private — core log path
// ---------------------------------------------------------------------------

LoggerResult LoggerManager::log(LogLevel level, const char* message)
{
    if (!m_initialized) return LoggerResult::NotInitialized;
    if (!m_enabled)     return LoggerResult::Disabled;
    if (message == nullptr) return LoggerResult::InvalidMessage;

    // Discard immediately before any snprintf work — keeps filtered messages
    // at zero formatting overhead, which matters for high-frequency debug logs.
    if (!shouldLog(level)) return LoggerResult::Success;

    const char* levelStr = levelToString(level);

    if (m_timestampEnabled)
    {
        // millis() returns unsigned long (32-bit on ESP32); %lu is correct.
        snprintf(m_buffer, LOG_BUFFER_SIZE,
                 "[%lu][%s] %s",
                 static_cast<unsigned long>(millis()), levelStr, message);
    }
    else
    {
        snprintf(m_buffer, LOG_BUFFER_SIZE, "[%s] %s", levelStr, message);
    }

    // snprintf guarantees null termination and truncation at LOG_BUFFER_SIZE-1.
    emit(level, m_buffer);
    ++m_logCount;
    return LoggerResult::Success;
}

void LoggerManager::emit(LogLevel level, const char* formattedMessage)
{
    if (m_outputCallback != nullptr)
    {
        // Pass the current message's level (not the configured minimum) so the
        // callback can apply its own severity-based routing or filtering.
        m_outputCallback(level, formattedMessage);
    }
    else
    {
        // Phase 1 default: Serial.println() appends the required newline.
        // Assumes Serial.begin() was called by the application before begin().
        Serial.println(formattedMessage);
    }
}

bool LoggerManager::shouldLog(LogLevel level) const
{
    // LogLevel::None disables all output.
    // A message passes when its numeric level is at or above the configured floor.
    if (m_logLevel == LogLevel::None) return false;
    return static_cast<uint8_t>(level) >= static_cast<uint8_t>(m_logLevel);
}

const char* LoggerManager::levelToString(LogLevel level) const
{
    switch (level)
    {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Private — EventBus integration
// ---------------------------------------------------------------------------

void LoggerManager::subscribeToEvents()
{
    if (m_eventBus == nullptr) return;

    // Subscribe to every concrete EventType in one range sweep.  This means
    // any EventType added between RelayStateChanged and Count in EventBus.h
    // is automatically logged without touching this file.
    const uint8_t first = static_cast<uint8_t>(EventType::RelayStateChanged);
    const uint8_t last  = static_cast<uint8_t>(EventType::Count);

    for (uint8_t i = first; i < last; ++i)
    {
        m_eventBus->subscribe(static_cast<EventType>(i), onEventBusEvent, this);
    }
}

// Static trampoline — EventBus stores plain function pointers; context carries
// the LoggerManager instance pointer so we can reach member state.
void LoggerManager::onEventBusEvent(const Event& event, void* context)
{
    LoggerManager* self = static_cast<LoggerManager*>(context);
    if (self != nullptr)
    {
        self->handleEvent(event);
    }
}

void LoggerManager::handleEvent(const Event& event)
{
    // Guard here rather than in subscribe() so the subscription stays alive
    // even when disabled; resuming via enable() immediately receives events.
    if (!m_initialized || !m_enabled) return;

    LogLevel    level   = LogLevel::Info;
    const char* message = eventTypeToMessage(event.type, level);

    if (message == nullptr) return;

    log(level, message);
}

const char* LoggerManager::eventTypeToMessage(EventType type, LogLevel& outLevel) const
{
    switch (type)
    {
        // Hardware
        case EventType::RelayStateChanged:
            outLevel = LogLevel::Debug;
            return "Relay state changed";

        case EventType::SwitchPressed:
            outLevel = LogLevel::Debug;
            return "Switch pressed";

        case EventType::SwitchReleased:
            outLevel = LogLevel::Debug;
            return "Switch released";

        // Network
        case EventType::WiFiConnected:
            outLevel = LogLevel::Info;
            return "WiFi connected";

        case EventType::WiFiDisconnected:
            outLevel = LogLevel::Warning;
            return "WiFi disconnected";

        case EventType::WiFiReconnecting:
            outLevel = LogLevel::Info;
            return "WiFi reconnecting";

        // Cloud
        case EventType::FirebaseReady:
            outLevel = LogLevel::Info;
            return "Firebase ready";

        case EventType::FirebaseDisconnected:
            outLevel = LogLevel::Warning;
            return "Firebase disconnected";

        case EventType::FirebaseError:
            outLevel = LogLevel::Error;
            return "Firebase error";

        // Synchronization
        case EventType::SyncStarted:
            outLevel = LogLevel::Debug;
            return "Sync started";

        case EventType::SyncCompleted:
            outLevel = LogLevel::Info;
            return "Sync completed";

        case EventType::SyncFailed:
            outLevel = LogLevel::Warning;
            return "Sync failed";

        // System
        case EventType::BootCompleted:
            outLevel = LogLevel::Info;
            return "Boot completed";

        case EventType::RestartRequested:
            outLevel = LogLevel::Warning;
            return "Restart requested";

        case EventType::ConfigurationChanged:
            outLevel = LogLevel::Info;
            return "Configuration changed";

        default:
            outLevel = LogLevel::Debug;
            return nullptr;  // unknown or reserved type; silently skip
    }
}
