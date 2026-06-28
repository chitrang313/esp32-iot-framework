#pragma once

// LoggerManager — centralized logging system for the ESP32 IoT Framework.
//
// Receives framework events from EventBus and converts them into formatted
// log messages.  Application code may also log directly via debug() / info()
// / warning() / error().
//
// Architecture:
//   Framework modules → EventBus → LoggerManager → Serial (Phase 1)
//
// Module independence: LoggerManager depends ONLY on EventBus.
// It never imports RelayManager, FirebaseManager, or any other framework module.
//
// Phase 1 output: Serial.println().
// Future outputs (SD, SPIFFS, MQTT, Firebase) added by registering a custom
// callback via setOutputCallback() — the public API stays unchanged.
//
// Typical usage:
//   LoggerManager logger;
//   void setup() {
//       Serial.begin(115200);
//       eventBus.begin();
//       logger.begin(eventBus);
//   }
//   void loop() {
//       eventBus.loop();
//       logger.loop();
//   }

#include <Arduino.h>
#include <stdint.h>
#include "../../Events/EventBus/EventBus.h"

// ---------------------------------------------------------------------------
// LogLevel
//
// Controls which messages are emitted.  Any message whose level is below the
// configured minimum is discarded before formatting, so filtered messages
// carry zero Serial overhead.
// ---------------------------------------------------------------------------
enum class LogLevel : uint8_t
{
    Debug   = 0,   ///< Verbose diagnostic output; highest noise, lowest threshold.
    Info    = 1,   ///< Normal operational events worth recording.
    Warning = 2,   ///< Recoverable anomalies that may need attention.
    Error   = 3,   ///< Failures that require immediate attention.
    None    = 4    ///< Suppress all output; useful for production builds.
};

// ---------------------------------------------------------------------------
// LoggerResult
//
// Returned by all public operations.  Never silently fail.
// ---------------------------------------------------------------------------
enum class LoggerResult : uint8_t
{
    Success,          ///< Operation completed successfully.
    Disabled,         ///< Manager is disabled; message was not emitted.
    NotInitialized,   ///< begin() has not been called.
    InvalidLevel,     ///< LogLevel value is out of range.
    InvalidMessage,   ///< Message pointer is null.
    Failed            ///< Unrecoverable internal error.
};

// ---------------------------------------------------------------------------
// LogOutputCallback
//
// Optional application-supplied output handler registered via
// setOutputCallback().  When set, it receives the fully formatted message
// string and REPLACES Serial output — the callback is responsible for all
// output routing, including Serial if still desired.
//
// formattedMessage includes the [timestamp][LEVEL] prefix but no trailing
// newline, so the callback can append one (or not) to suit its destination.
// ---------------------------------------------------------------------------
using LogOutputCallback = void (*)(LogLevel level, const char* formattedMessage);

// ---------------------------------------------------------------------------
// LoggerManager
// ---------------------------------------------------------------------------

class LoggerManager
{
public:

    /// Maximum characters (including null terminator) of the internal format
    /// buffer.  prefix ([4294967295][WARNING] ) + message must fit within this.
    static constexpr uint16_t LOG_BUFFER_SIZE = 256u;

    // -----------------------------------------------------------------------
    // Construction
    //
    // Zero-initialises members.  No I/O, no Serial, no EventBus access.
    // -----------------------------------------------------------------------
    LoggerManager();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises the manager and subscribes to all EventBus event types.
     *
     * Must be called once from setup() after Serial.begin() and eventBus.begin().
     * Safe to call again to re-subscribe if EventBus was restarted.
     *
     * @param eventBus Reference to the application's EventBus instance.
     */
    void begin(EventBus& eventBus);

    /**
     * @brief Drives the logger main loop.
     *
     * Phase 1: no-op — log messages are emitted synchronously in the EventBus
     * callback.  Reserved for future output queuing or deferred Serial writes.
     */
    void loop();

    /**
     * @brief Re-enables the manager after a disable() call.
     */
    void enable();

    /**
     * @brief Suspends all log output.
     *
     * EventBus subscription remains active; events received while disabled are
     * silently discarded.  State and configuration are preserved for resume.
     */
    void disable();

    /**
     * @brief Returns whether the manager is currently accepting log calls.
     * @return true when enabled, false when disabled.
     */
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Logging
    //
    // Each method maps to a fixed LogLevel.  Messages whose level falls below
    // the configured minimum are discarded before any string formatting occurs.
    // begin() must be called before any of these functions.
    // -----------------------------------------------------------------------

    /**
     * @brief Emits a DEBUG-level message.
     *
     * @param message Null-terminated C string.  Must not be null.
     * @return Success, Disabled, NotInitialized, or InvalidMessage.
     */
    LoggerResult debug(const char* message);

    /**
     * @brief Emits an INFO-level message.
     *
     * @param message Null-terminated C string.  Must not be null.
     * @return Success, Disabled, NotInitialized, or InvalidMessage.
     */
    LoggerResult info(const char* message);

    /**
     * @brief Emits a WARNING-level message.
     *
     * @param message Null-terminated C string.  Must not be null.
     * @return Success, Disabled, NotInitialized, or InvalidMessage.
     */
    LoggerResult warning(const char* message);

    /**
     * @brief Emits an ERROR-level message.
     *
     * @param message Null-terminated C string.  Must not be null.
     * @return Success, Disabled, NotInitialized, or InvalidMessage.
     */
    LoggerResult error(const char* message);

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the minimum log level for output.
     *
     * Messages below this level are dropped immediately.
     * LogLevel::None suppresses all output.
     * May be called before begin() to configure level before first log.
     *
     * @param level New minimum level.
     * @return Success, or InvalidLevel if the value is out of range.
     */
    LoggerResult setLogLevel(LogLevel level);

    /**
     * @brief Returns the current minimum log level.
     * @return Current LogLevel.
     */
    LogLevel getLogLevel() const;

    /**
     * @brief Prepends [millis()] to every emitted log message.
     *
     * Enabled by default.  Disabling reduces per-message Serial overhead when
     * an external system adds its own timestamps.
     */
    void enableTimestamp();

    /**
     * @brief Suppresses the [millis()] prefix from emitted log messages.
     */
    void disableTimestamp();

    // -----------------------------------------------------------------------
    // Output
    // -----------------------------------------------------------------------

    /**
     * @brief Registers a custom output handler, replacing the default Serial output.
     *
     * When set, the callback receives the fully formatted message string instead
     * of Serial.println().  Pass nullptr to revert to Serial output.
     *
     * Useful for redirecting logs to SD, MQTT, Firebase, or a display without
     * changing any other part of the API.
     *
     * @param callback Function pointer to the custom handler, or nullptr to use Serial.
     */
    void setOutputCallback(LogOutputCallback callback);

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the total number of messages successfully emitted since begin().
     *
     * Only counts messages that passed the log level filter and were written to
     * an output target.  Dropped (filtered) messages are not counted.
     *
     * @return Cumulative emitted message count.
     */
    uint32_t getLogCount() const;

    /**
     * @brief Resets the emitted message counter to zero.
     */
    void clearLogCount();

private:

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    bool              m_initialized;
    bool              m_enabled;
    bool              m_timestampEnabled;
    LogLevel          m_logLevel;
    EventBus*         m_eventBus;
    LogOutputCallback m_outputCallback;
    uint32_t          m_logCount;

    // -----------------------------------------------------------------------
    // Format buffer
    //
    // Member rather than stack allocation to avoid 256-byte stack pressure on
    // every log call.  Safe in single-threaded Arduino execution.
    // -----------------------------------------------------------------------
    char m_buffer[LOG_BUFFER_SIZE];

    // -----------------------------------------------------------------------
    // Private helpers — core path
    // -----------------------------------------------------------------------

    /// Central log function called by all public debug/info/warning/error methods.
    /// Validates, filters, formats, and emits; increments m_logCount on success.
    LoggerResult log(LogLevel level, const char* message);

    /// Writes formattedMessage to the registered output target (callback or Serial).
    /// level is the level of this specific message, forwarded to the custom callback
    /// so it can apply its own filtering or routing per message severity.
    void emit(LogLevel level, const char* formattedMessage);

    /// Returns true when level is at or above the configured minimum.
    /// Inline fast path — called before any string work to short-circuit filtered messages.
    bool shouldLog(LogLevel level) const;

    /// Maps LogLevel to its fixed-width label string used in formatting.
    const char* levelToString(LogLevel level) const;

    // -----------------------------------------------------------------------
    // Private helpers — EventBus integration
    // -----------------------------------------------------------------------

    /// Subscribes to every known EventType in a range loop so new types added
    /// to EventType::Count are automatically picked up without code changes here.
    void subscribeToEvents();

    /// Static trampoline registered with EventBus.  Required because EventBus
    /// stores plain function pointers (no std::function / heap closure).
    /// Recovers the LoggerManager instance from the context pointer.
    static void onEventBusEvent(const Event& event, void* context);

    /// Handles a dispatched Event: maps type → log level + message, then calls log().
    void handleEvent(const Event& event);

    /// Maps an EventType to a human-readable message and the appropriate LogLevel.
    /// Returns nullptr when the type has no registered message (silently ignored).
    const char* eventTypeToMessage(EventType type, LogLevel& outLevel) const;
};
