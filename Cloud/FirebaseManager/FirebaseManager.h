#pragma once

// FirebaseManager — the single owner of all Firebase Realtime Database
// communication for this ESP32 IoT Framework.
//
// No other module may call Firebase library functions directly.
// Application code must never use:
//   FirebaseData, FirebaseAuth, FirebaseConfig, FirebaseJson,
//   Firebase.RTDB, Firebase.ready(), Firebase.signUp(), Firebase.begin()
//
// Dependencies (must be installed in Arduino/PlatformIO library manager):
//   Firebase Arduino Client Library for ESP8266 and ESP32 by Mobizt (v4.4.x)
//   ArduinoJson by bblanchon       (v7+)
//
// NOTE: <Firebase_ESP_Client.h> is included here because FirebaseData,
// FirebaseAuth, and FirebaseConfig are stored as value-type private members.
// PIMPL would hide them but requires dynamic allocation — forbidden in this
// framework.  Application code must still never use Firebase types directly.
//
// ---------------------------------------------------------------------------
// THREADING EXCEPTION (deliberate deviation from AGENT_INSTRUCTIONS.md)
// ---------------------------------------------------------------------------
// This module is the ONE place in the framework that spawns a FreeRTOS task.
// It is "explicitly required" (per AGENT_INSTRUCTIONS Thread Model clause) and
// the reason is structural, not stylistic:
//
//   The Mobizt library's Firebase.begin() performs the *first* email/password
//   authentication SYNCHRONOUSLY inside an internal no-timeout while-loop
//   (FirebaseCore::tokenProcessingTask). On bad credentials, a disabled
//   sign-in provider, or a rate-limit, getIdToken() returns false and the loop
//   retries the auth endpoint forever — one HTTPS request per iteration with
//   no throttle. That request storm makes Google return
//   TOO_MANY_ATTEMPTS_TRY_LATER (a self-sustaining ban) AND, because begin()
//   never returns, freezes the Arduino loop() so every other module dies.
//
//   The loop cannot be broken from the calling thread. The only way to bound
//   it is to run begin() in a separate task and delete that task on the first
//   token error. That is exactly what this module does, and ONLY for the first
//   authentication. All later reconnects use the cheap, storm-safe
//   Firebase.ready() polling path on the main thread.
//
// The task is single, short-lived, pinned to core 0 (Arduino loop runs on
// core 1), and synchronised with the main loop using volatile flags only — no
// mutexes. The main loop is the SOLE deleter of the task to avoid any
// double-free; the task self-parks after finishing instead of self-deleting.

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Compile-time limits
// ---------------------------------------------------------------------------

// Maximum length for credentials: API key, database URL, email, password.
static constexpr uint16_t FIREBASE_MAX_CONFIG_LEN = 256;

// Maximum length for a database path string.
static constexpr uint16_t FIREBASE_MAX_PATH_LEN = 128;

// Maximum length for string and JSON payload values in the offline queue.
static constexpr uint16_t FIREBASE_MAX_VALUE_LEN = 512;

// Depth of the offline write queue (FIFO ring buffer).
// Kept small — 50 × 649 bytes = 32 KB of static BSS RAM which can cause
// boot-time memory exhaustion alongside Firebase library heap usage.
static constexpr uint8_t FIREBASE_MAX_QUEUE_OPERATIONS = 10;

// Maximum number of simultaneously subscribed database paths.
static constexpr uint8_t FIREBASE_MAX_SUBSCRIPTIONS = 8;

// Maximum connection/auth retry attempts before declaring Error.
static constexpr uint8_t FIREBASE_MAX_RETRY_COUNT = 5;

// ---------------------------------------------------------------------------
// FirebaseState
//
// Public view of the connection lifecycle.  Only FirebaseManager may write
// this — application code is read-only via getState().
// ---------------------------------------------------------------------------
enum class FirebaseState : uint8_t
{
    NotConfigured,   // configure() not yet called (or failed)
    Configured,      // credentials stored; begin() not called
    Connecting,      // waiting for WiFi and/or initiating Firebase session
    Authenticating,  // Firebase.begin() running (first auth) or ready() polling
    Connected,       // authenticated; transitional before stream/queue ready
    Ready,           // fully operational; reads/writes/streams active
    Disconnected,    // WiFi or Firebase session lost; auto-recovery pending
    Disabled,        // module disabled via disable(); no operations run
    Error            // unrecoverable (bad credentials / rate-limit); reset required
};

// ---------------------------------------------------------------------------
// FirebaseResult
//
// Returned by every public API call.  Never silently fail.
// ---------------------------------------------------------------------------
enum class FirebaseResult : uint8_t
{
    Success,
    Queued,               // operation accepted into offline queue (not yet sent)
    Busy,                 // previous operation still in progress
    InvalidPath,          // null or empty path
    InvalidValue,         // null or invalid value argument
    InvalidType,          // unsupported data type for this operation
    NotConfigured,        // configure() has not been called successfully
    NotConnected,         // WiFi or Firebase not ready
    AuthenticationFailed, // wrong credentials, disabled provider, bad API key
    RateLimited,          // auth temporarily blocked (TOO_MANY_ATTEMPTS); cooldown required
    PermissionDenied,     // Firebase security rules blocked the operation
    Timeout,              // operation did not complete within deadline
    Disabled,             // module is disabled
    ConfigurationClosed,  // configure() called after begin()
    AlreadyConfigured,    // configure() called a second time before begin()
    AlreadyConnected,     // connect() called when already connected
    AlreadyDisconnected,  // disconnect() called when already disconnected
    SubscriptionExists,   // subscribe() called for a path already subscribed
    SubscriptionNotFound, // unsubscribe() called for a path not subscribed
    QueueFull,            // offline queue at capacity; operation dropped
    UnknownError          // unexpected Firebase library error
};

// ---------------------------------------------------------------------------
// FirebaseEvent
//
// Published via the single onEvent() callback.  Every Firebase activity
// produces an event; the application decides how to respond.
// ---------------------------------------------------------------------------
enum class FirebaseEvent : uint8_t
{
    Connected,
    Disconnected,
    Authenticated,
    Ready,
    ReadCompleted,
    WriteCompleted,
    StreamUpdated,
    SubscriptionAdded,
    SubscriptionRemoved,
    QueueStarted,
    QueueFlushed,
    QueueEmpty,
    QueueFull,
    QueueOperationFailed,
    RetryStarted,
    RetryCompleted,
    RetryLimitReached,
    Error
};

// ---------------------------------------------------------------------------
// FirebaseDataType
//
// Carried in callbacks so the application knows what value type to read.
// ---------------------------------------------------------------------------
enum class FirebaseDataType : uint8_t
{
    Boolean,
    Integer,
    Float,
    Double,
    String,
    Json,
    Array,
    Unknown
};

// ---------------------------------------------------------------------------
// FirebaseManager
// ---------------------------------------------------------------------------

class FirebaseManager
{
public:

    // -----------------------------------------------------------------------
    // Event callback
    //
    // One registration for all events.  Raw function pointer — no heap.
    //
    //   event   — what happened
    //   result  — Success or the specific error
    //   type    — data type involved (Unknown when not applicable)
    //   path    — database path involved (empty string when not applicable)
    //   context — opaque pointer registered with onEvent()
    // -----------------------------------------------------------------------
    using EventCallback =
        void (*)(FirebaseEvent    event,
                 FirebaseResult   result,
                 FirebaseDataType type,
                 const char*      path,
                 void*            context);

    // -----------------------------------------------------------------------
    // Construction
    //
    // Default constructor only.  No network operations, no Firebase calls.
    // All initialisation happens in configure() and begin().
    // -----------------------------------------------------------------------
    FirebaseManager();

    // -----------------------------------------------------------------------
    // Configuration  (must be called before begin())
    //
    // Stores credentials internally.  Does NOT connect or authenticate.
    // Validation:
    //   - Any null or empty argument → InvalidValue
    //   - Called after begin()       → ConfigurationClosed
    //   - Called a second time       → AlreadyConfigured
    //
    // Credentials are never logged or exposed through any other API.
    // -----------------------------------------------------------------------
    FirebaseResult configure(const char* apiKey,
                              const char* databaseUrl,
                              const char* userEmail,
                              const char* userPassword);

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    // Call once from setup() after configure().
    // Seals configuration and starts the connection state machine.  The actual
    // connection and authentication happen asynchronously in loop().
    void begin();

    // Call from every loop() iteration.
    // Drives the connection state machine, token refresh, stream polling,
    // offline queue processing, and event dispatch.  Never blocks.
    void loop();

    // -----------------------------------------------------------------------
    // Enable / disable  (module-level gate)
    //
    // disable() stops all loop() activity and rejects all operations.
    // Hardware and library state are preserved.
    // enable() resumes normal operation from the last known state.
    // -----------------------------------------------------------------------

    void enable();
    void disable();
    bool isEnabled() const;

    // -----------------------------------------------------------------------
    // Connection control  (non-blocking)
    //
    // connect()    — request a connection attempt from the current state.
    // disconnect() — tear down the Firebase session; auto-reconnect suspended.
    // reconnect()  — force a full re-authentication (clears Error and backoff).
    //                Use this to recover after fixing credentials or after a
    //                rate-limit cooldown has elapsed.
    // -----------------------------------------------------------------------

    FirebaseResult connect();
    FirebaseResult disconnect();
    FirebaseResult reconnect();

    // -----------------------------------------------------------------------
    // Status queries  (const; always safe)
    // -----------------------------------------------------------------------

    bool          isConfigured()    const;
    bool          isConnected()     const;
    bool          isAuthenticated() const;
    bool          isReady()         const;
    FirebaseState getState()        const;

    // -----------------------------------------------------------------------
    // Read API  (overloaded get)
    //
    // Returns FirebaseResult::NotConnected when not Ready.
    // Reads are NOT queued offline — they are rejected if not Ready, since
    // a stale read result has no value.
    //
    // String overload requires a caller-provided buffer (embedded-safe, no
    // dynamic allocation).  bufferLen must include space for the null
    // terminator.
    // -----------------------------------------------------------------------

    FirebaseResult get(const char* path, bool&   outValue);
    FirebaseResult get(const char* path, int&    outValue);
    FirebaseResult get(const char* path, float&  outValue);
    FirebaseResult get(const char* path, double& outValue);
    FirebaseResult get(const char* path, char*   outBuffer, size_t bufferLen);
    FirebaseResult get(const char* path, JsonDocument& outDoc);

    // -----------------------------------------------------------------------
    // Write API  (overloaded set)
    //
    // If Ready: sends immediately, returns Success or error.
    // If not Ready (WiFi loss, auth pending, retry delay): enqueues the
    // operation and returns Queued — automatically sent when Ready.
    // If queue full: returns QueueFull.
    // -----------------------------------------------------------------------

    FirebaseResult set(const char* path, bool              value);
    FirebaseResult set(const char* path, int               value);
    FirebaseResult set(const char* path, float             value);
    FirebaseResult set(const char* path, double            value);
    FirebaseResult set(const char* path, const char*       value);
    FirebaseResult set(const char* path, const JsonDocument& doc);

    // -----------------------------------------------------------------------
    // Streaming  (realtime subscriptions)
    //
    // subscribe()      — register a database path for realtime updates.
    // unsubscribe()    — remove a subscription.
    // unsubscribeAll() — remove all subscriptions.
    //
    // Phase 1 streams one path at a time.  All registered paths receive
    // FirebaseEvent::StreamUpdated callbacks when their value changes.
    // -----------------------------------------------------------------------

    FirebaseResult subscribe(const char* path);
    FirebaseResult unsubscribe(const char* path);
    FirebaseResult unsubscribeAll();

    // -----------------------------------------------------------------------
    // Event callback registration
    //
    // Expose ONE callback for all Firebase events.  Pass nullptr to
    // unregister.  context is an optional opaque pointer forwarded to the
    // callback — useful for member-function dispatch.
    // -----------------------------------------------------------------------

    void onEvent(EventCallback callback, void* context = nullptr);

private:

    // -----------------------------------------------------------------------
    // Internal timing constants
    // -----------------------------------------------------------------------

    // How long to wait between retry attempts (ms) — doubles per attempt.
    static constexpr uint32_t RETRY_BASE_DELAY_MS = 2000UL;
    static constexpr uint32_t RETRY_MAX_DELAY_MS  = 32000UL;

    // How long to wait for the async auth task before declaring a timeout.
    static constexpr uint32_t AUTH_TIMEOUT_MS     = 30000UL;

    // How long Firebase.ready() may be false before we declare disconnected.
    // Covers transient false during token refresh.
    static constexpr uint32_t READY_GRACE_PERIOD_MS = 5000UL;

    // How often to check Firebase.ready() and WiFi when in Ready state (ms).
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 3000UL;

    // How often to advance the offline queue (one entry per tick).
    static constexpr uint32_t QUEUE_TICK_INTERVAL_MS = 200UL;

    // Minimum interval between Firebase.ready() calls during reconnect auth.
    // Firebase.ready() can block briefly during SSL + token refresh; throttling
    // prevents starving other modules.
    static constexpr uint32_t READY_CHECK_INTERVAL_MS = 500UL;

    // Minimum interval between Firebase.RTDB.readStream() calls.
    // Provides 20 Hz stream responsiveness without blocking every loop() tick.
    static constexpr uint32_t STREAM_READ_INTERVAL_MS = 50UL;

    // -----------------------------------------------------------------------
    // Async auth-task constants  (see THREADING EXCEPTION note at top)
    // -----------------------------------------------------------------------

    // NTP must succeed before the first auth: Firebase TLS validates the server
    // certificate against the wall clock, and the JWT/token flow needs real
    // time.  We sync it ourselves with a hard timeout so the library's own
    // no-timeout NTP wait can never hang.
    static constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 15000UL;

    // Epoch below this (2021-01-01) means the clock is not yet set.
    static constexpr uint32_t MIN_VALID_EPOCH = 1609459200UL;

    // Auth-task stack (bytes on ESP32) — TLS + NTP have a deep call chain.
    static constexpr uint32_t INIT_TASK_STACK_BYTES = 16384UL;

    // Auth task runs on core 0; the Arduino loop runs on core 1.
    static constexpr BaseType_t INIT_TASK_CORE = 0;

    // Make the library raise its first token-error callback immediately so we
    // can abort the storming task after a single failed attempt.
    static constexpr uint32_t TOKEN_GENERATION_ERROR_MS  = 1000UL;
    static constexpr uint32_t SERVER_RESPONSE_TIMEOUT_MS = 10000UL;
    static constexpr uint32_t SOCKET_CONNECT_TIMEOUT_MS  = 5000UL;

    // BSSL receive/transmit buffer sizes for the RTDB data + stream clients.
    // The default (~16 KB each) would consume ~32 KB of heap across two
    // FirebaseData objects; these minimums match the B805 reference sketch.
    static constexpr int BSSL_RX_BYTES = 2048;
    static constexpr int BSSL_TX_BYTES = 1024;

    // Captured token-error message buffer length.
    static constexpr uint16_t TOKEN_ERR_MSG_LEN = 80;

    // -----------------------------------------------------------------------
    // Internal enums
    // -----------------------------------------------------------------------

    enum class QueuedOpType : uint8_t
    {
        None = 0,
        WriteBool,
        WriteInt,
        WriteFloat,
        WriteDouble,
        WriteString,
        WriteJson
    };

    // Outcome reported by the async auth task back to the main loop.
    enum class InitOutcome : uint8_t
    {
        Pending,    // task still working
        Success,    // Firebase.begin() returned → authentication succeeded
        NtpFailed   // NTP time sync timed out → treat as a transient failure
    };

    // Classification of a token-generation error, deciding the recovery policy.
    enum class TokenErrorClass : uint8_t
    {
        None,
        Transient,   // network/TCP — bounded backoff retry is safe
        Credential,  // wrong key/email/password/disabled provider — terminal
        Quota        // TOO_MANY_ATTEMPTS rate-limit — terminal (avoid re-ban)
    };

    // Fixed-size queue entry.  The union holds typed values; stringVal also
    // holds serialised JSON for WriteJson entries.
    struct QueueEntry
    {
        char         path[FIREBASE_MAX_PATH_LEN];
        QueuedOpType opType;
        union
        {
            bool   boolVal;
            int    intVal;
            float  floatVal;
            double doubleVal;
        };
        char stringVal[FIREBASE_MAX_VALUE_LEN];
    };

    // -----------------------------------------------------------------------
    // Member variables
    // -----------------------------------------------------------------------

    // Credentials (stored once by configure(), never exposed externally).
    char m_apiKey     [FIREBASE_MAX_CONFIG_LEN];
    char m_databaseUrl[FIREBASE_MAX_CONFIG_LEN];
    char m_userEmail  [FIREBASE_MAX_CONFIG_LEN];
    char m_userPassword[FIREBASE_MAX_CONFIG_LEN];

    // Lifecycle flags.
    bool m_configured;           // configure() succeeded
    bool m_configurationClosed;  // begin() called; configure() rejected
    bool m_began;                // begin() called
    bool m_enabled;              // module-level gate
    bool m_fbBegun;              // Firebase.begin() has completed successfully once
    bool m_manualDisconnect;     // disconnect() called; suppress auto-reconnect

    // State machine.
    FirebaseState m_state;
    FirebaseState m_preDisableState; // restored on enable()

    // Connection / retry timing.
    uint32_t m_connectStartMs;    // when the current auth attempt started
    uint32_t m_lastReadyMs;       // last millis() when Firebase.ready() == true
    uint32_t m_retryTimerMs;      // when the current backoff delay started
    uint32_t m_retryDelayMs;      // current backoff duration
    uint8_t  m_retryCount;        // consecutive failed attempts

    // Heartbeat and queue scheduling.
    uint32_t m_lastHeartbeatMs;
    uint32_t m_lastQueueTickMs;
    uint32_t m_lastReadyCheckMs;  // throttle Firebase.ready() during reconnect
    uint32_t m_lastStreamReadMs;  // throttle readStream() polling

    // Async auth task (first authentication only — see THREADING EXCEPTION).
    TaskHandle_t         m_initTaskHandle;   // handle of the running auth task
    volatile InitOutcome m_initOutcome;      // result written by the task
    bool                 m_initTaskLaunched; // a task is currently outstanding

    // Offline write queue (FIFO ring buffer).
    QueueEntry m_queue[FIREBASE_MAX_QUEUE_OPERATIONS];
    uint8_t    m_queueHead;          // index of next entry to dequeue
    uint8_t    m_queueTail;          // index of next empty slot
    uint8_t    m_queueCount;         // number of entries in queue
    bool       m_queueFlushStarted;  // true after QueueStarted fires; cleared on flush or disconnect

    // Realtime subscriptions.
    char    m_subscriptions[FIREBASE_MAX_SUBSCRIPTIONS][FIREBASE_MAX_PATH_LEN];
    uint8_t m_subscriptionCount;
    uint8_t m_activeStreamIndex;  // index of the currently streamed path
    bool    m_streamBegun;        // Firebase.RTDB.beginStream() called

    // Event callback.
    EventCallback m_eventCallback;
    void*         m_eventContext;

    // Firebase library objects.  These are private: application code must
    // never access them.  They live here (not heap) to satisfy the no-new
    // rule while still encapsulating the library.
    FirebaseData   m_fbData;    // used for get() / set() operations
    FirebaseData   m_fbStream;  // dedicated stream connection
    FirebaseAuth   m_fbAuth;
    FirebaseConfig m_fbConfig;

    // -----------------------------------------------------------------------
    // Static state shared with the C-style token-status callback.
    //
    // The Mobizt library's token_status_callback is a plain function pointer
    // (no context argument), so the bridge to the instance goes through these
    // statics.  Only one FirebaseManager owns the single global Firebase object
    // at a time, so a single instance pointer is sufficient.  Flags are
    // volatile because the callback runs on the auth task (core 0) while the
    // main loop (core 1) reads them.  The message is written BEFORE the flag is
    // raised so a reader that sees the flag also sees a complete message.
    // -----------------------------------------------------------------------
    static FirebaseManager* s_instance;
    static volatile bool    s_tokenError;
    static volatile int     s_tokenErrCode;
    static char             s_tokenErrMsg[TOKEN_ERR_MSG_LEN];

    // -----------------------------------------------------------------------
    // Private helpers — connection state machine
    // -----------------------------------------------------------------------

    void tickConnection(uint32_t nowMs);
    void tickHeartbeat(uint32_t nowMs);
    void tickStream(uint32_t nowMs);
    void tickQueue(uint32_t nowMs);

    // Drive the first, storm-protected authentication (async task path).
    void tickFirstAuth(uint32_t nowMs);

    // Drive a reconnect authentication (cheap Firebase.ready() polling path).
    void tickReconnectAuth(uint32_t nowMs);

    // Promote the state machine to Ready and emit the matching events.
    void enterReady(uint32_t nowMs);

    // -----------------------------------------------------------------------
    // Private helpers — async auth task (see THREADING EXCEPTION)
    // -----------------------------------------------------------------------

    // Configure the Firebase library structs and spawn the auth task.
    // Returns false if the task could not be created (e.g. out of heap).
    bool launchInitTask();

    // Delete the auth task (main loop is the SOLE deleter) and reset bookkeeping.
    void deleteInitTask();

    // FreeRTOS entry point — forwards to runInitTask() on the instance.
    static void initTaskTrampoline(void* param);

    // Task body: NTP sync, then Firebase.begin(); reports via m_initOutcome.
    void runInitTask();

    // Blocking NTP sync with a hard timeout.  Runs only inside the auth task.
    static bool syncTimeWithTimeout(uint32_t timeoutMs);

    // Classify the most recent captured token error.
    TokenErrorClass classifyTokenError() const;

    // The library's token-status callback (plain function pointer).
    static void tokenStatusCallbackStatic(TokenInfo info);

    // Advance the state machine toward the next state.
    void transitionTo(FirebaseState newState);

    // Schedule the next reconnect attempt with exponential backoff.
    void scheduleRetry();

    // Reset retry counters when a connection succeeds.
    void resetRetry();

    // -----------------------------------------------------------------------
    // Private helpers — operations
    // -----------------------------------------------------------------------

    // Pre-flight check for read/write: enabled + configured + ready.
    FirebaseResult validateReady() const;

    // Pre-flight check for path argument.
    static FirebaseResult validatePath(const char* path);

    // Translate a Firebase library HTTP result code to FirebaseResult.
    static FirebaseResult mapHttpCode(int httpCode);

    // Execute one write entry from the queue.  Called by tickQueue().
    bool executeQueueEntry(const QueueEntry& entry);

    // -----------------------------------------------------------------------
    // Private helpers — queue
    // -----------------------------------------------------------------------

    // Add a write operation to the tail of the FIFO queue.
    FirebaseResult enqueueWrite(const QueueEntry& entry);

    // Remove and return the oldest queue entry.  Returns false if empty.
    bool dequeueWrite(QueueEntry& outEntry);

    // -----------------------------------------------------------------------
    // Private helpers — subscriptions
    // -----------------------------------------------------------------------

    // Find the slot index for a path, or -1 if not found.
    int findSubscriptionIndex(const char* path) const;

    // Start streaming the subscription at m_activeStreamIndex.
    void beginStream();

    // Advance to the next subscription path after the current stream.
    void advanceStreamIndex();

    // -----------------------------------------------------------------------
    // Private helpers — events
    // -----------------------------------------------------------------------

    void fireEvent(FirebaseEvent    event,
                   FirebaseResult   result   = FirebaseResult::Success,
                   FirebaseDataType type     = FirebaseDataType::Unknown,
                   const char*      path     = "");

    // -----------------------------------------------------------------------
    // Private helpers — WiFi
    // -----------------------------------------------------------------------

    static bool isWiFiConnected();
};
