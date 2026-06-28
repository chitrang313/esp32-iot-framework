#include "FirebaseManager.h"
#include <WiFi.h>

// ---------------------------------------------------------------------------
// Static member definitions
//
// Bridge between the Mobizt library's plain-function token_status_callback and
// this instance.  See the THREADING EXCEPTION note in the header.
// ---------------------------------------------------------------------------

FirebaseManager* FirebaseManager::s_instance     = nullptr;
volatile bool    FirebaseManager::s_tokenError   = false;
volatile int     FirebaseManager::s_tokenErrCode = 0;
char             FirebaseManager::s_tokenErrMsg[FirebaseManager::TOKEN_ERR_MSG_LEN] = { 0 };

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

FirebaseManager::FirebaseManager()
    : m_apiKey{}
    , m_databaseUrl{}
    , m_userEmail{}
    , m_userPassword{}
    , m_configured(false)
    , m_configurationClosed(false)
    , m_began(false)
    , m_enabled(true)
    , m_fbBegun(false)
    , m_manualDisconnect(false)
    , m_state(FirebaseState::NotConfigured)
    , m_preDisableState(FirebaseState::NotConfigured)
    , m_connectStartMs(0)
    , m_lastReadyMs(0)
    , m_retryTimerMs(0)
    , m_retryDelayMs(RETRY_BASE_DELAY_MS)
    , m_retryCount(0)
    , m_lastHeartbeatMs(0)
    , m_lastQueueTickMs(0)
    , m_lastReadyCheckMs(0)
    , m_lastStreamReadMs(0)
    , m_initTaskHandle(nullptr)
    , m_initOutcome(InitOutcome::Pending)
    , m_initTaskLaunched(false)
    , m_queue{}
    , m_queueHead(0)
    , m_queueTail(0)
    , m_queueCount(0)
    , m_queueFlushStarted(false)
    , m_subscriptions{}
    , m_subscriptionCount(0)
    , m_activeStreamIndex(0)
    , m_streamBegun(false)
    , m_eventCallback(nullptr)
    , m_eventContext(nullptr)
{
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::configure(const char* apiKey,
                                           const char* databaseUrl,
                                           const char* userEmail,
                                           const char* userPassword)
{
    if (m_configurationClosed)
    {
        return FirebaseResult::ConfigurationClosed;
    }

    if (m_configured)
    {
        return FirebaseResult::AlreadyConfigured;
    }

    // Reject any null or empty credential — a partial configuration would
    // produce confusing auth errors far from the source.
    if (apiKey     == nullptr || apiKey[0]      == '\0' ||
        databaseUrl == nullptr || databaseUrl[0] == '\0' ||
        userEmail   == nullptr || userEmail[0]   == '\0' ||
        userPassword == nullptr || userPassword[0] == '\0')
    {
        return FirebaseResult::InvalidValue;
    }

    strncpy(m_apiKey,      apiKey,      FIREBASE_MAX_CONFIG_LEN - 1);
    strncpy(m_databaseUrl, databaseUrl, FIREBASE_MAX_CONFIG_LEN - 1);
    strncpy(m_userEmail,   userEmail,   FIREBASE_MAX_CONFIG_LEN - 1);
    strncpy(m_userPassword, userPassword, FIREBASE_MAX_CONFIG_LEN - 1);

    m_apiKey[FIREBASE_MAX_CONFIG_LEN - 1]      = '\0';
    m_databaseUrl[FIREBASE_MAX_CONFIG_LEN - 1] = '\0';
    m_userEmail[FIREBASE_MAX_CONFIG_LEN - 1]   = '\0';
    m_userPassword[FIREBASE_MAX_CONFIG_LEN - 1] = '\0';

    m_configured = true;

    transitionTo(FirebaseState::Configured);

    return FirebaseResult::Success;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void FirebaseManager::begin()
{
    if (!m_configured || m_began)
    {
        return;
    }

    m_began               = true;
    m_configurationClosed = true;

    // Credentials and library tuning are applied later, in launchInitTask(),
    // immediately before the auth task runs Firebase.begin().  The connection
    // itself proceeds asynchronously from loop().
    transitionTo(FirebaseState::Connecting);
}

void FirebaseManager::loop()
{
    if (!m_enabled || !m_began)
    {
        return;
    }

    const uint32_t nowMs = millis();

    tickConnection(nowMs);
    tickHeartbeat(nowMs);
    tickStream(nowMs);
    tickQueue(nowMs);
}

// ---------------------------------------------------------------------------
// Enable / disable
// ---------------------------------------------------------------------------

void FirebaseManager::enable()
{
    if (m_enabled)
    {
        return;
    }

    m_enabled = true;

    // Restore the state we were in before disable() froze it.
    transitionTo(m_preDisableState);
}

void FirebaseManager::disable()
{
    if (!m_enabled)
    {
        return;
    }

    // If an auth task is mid-flight, tear it down and arrange to restart the
    // connection cleanly on the next enable() rather than resuming a state that
    // referenced a task that no longer exists.
    FirebaseState restore = m_state;
    if (m_initTaskLaunched)
    {
        deleteInitTask();
        restore = FirebaseState::Connecting;
    }

    m_preDisableState = restore;
    m_enabled         = false;

    transitionTo(FirebaseState::Disabled);
}

bool FirebaseManager::isEnabled() const
{
    return m_enabled;
}

// ---------------------------------------------------------------------------
// Connection control
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::connect()
{
    if (!m_enabled)  { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    if (m_state == FirebaseState::Ready || m_state == FirebaseState::Connected)
    {
        return FirebaseResult::AlreadyConnected;
    }

    m_manualDisconnect = false;
    resetRetry();
    transitionTo(FirebaseState::Connecting);

    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::disconnect()
{
    if (!m_enabled)  { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    if (m_state == FirebaseState::Disconnected)
    {
        return FirebaseResult::AlreadyDisconnected;
    }

    // Stop any in-flight auth task BEFORE touching the Firebase library from
    // this thread — the task owns the library while it runs.
    if (m_initTaskLaunched)
    {
        deleteInitTask();
    }

    m_manualDisconnect = true;

    // Close the active stream so it does not keep the TCP connection open.
    // (m_streamBegun is only ever true in the Ready state, where no auth task
    // is running, so this library call cannot race the task.)
    if (m_streamBegun)
    {
        Firebase.RTDB.endStream(&m_fbStream);
        m_streamBegun = false;
    }

    transitionTo(FirebaseState::Disconnected);
    fireEvent(FirebaseEvent::Disconnected);

    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::reconnect()
{
    if (!m_enabled)  { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    // Tear down any in-flight task and the stream before resetting state.
    if (m_initTaskLaunched)
    {
        deleteInitTask();
    }
    if (m_streamBegun)
    {
        Firebase.RTDB.endStream(&m_fbStream);
        m_streamBegun = false;
    }

    m_manualDisconnect = false;
    resetRetry();
    s_tokenError = false;

    // Force a full, storm-protected re-authentication.  This is the supported
    // way to recover from a terminal Error (bad credentials fixed, or a
    // rate-limit cooldown elapsed).
    m_fbBegun = false;

    transitionTo(FirebaseState::Connecting);
    fireEvent(FirebaseEvent::RetryStarted);

    return FirebaseResult::Success;
}

// ---------------------------------------------------------------------------
// Status queries
// ---------------------------------------------------------------------------

bool FirebaseManager::isConfigured() const
{
    return m_configured;
}

bool FirebaseManager::isConnected() const
{
    return m_state == FirebaseState::Connected
        || m_state == FirebaseState::Ready;
}

bool FirebaseManager::isAuthenticated() const
{
    return m_state == FirebaseState::Connected
        || m_state == FirebaseState::Ready;
}

bool FirebaseManager::isReady() const
{
    return m_state == FirebaseState::Ready;
}

FirebaseState FirebaseManager::getState() const
{
    return m_state;
}

// ---------------------------------------------------------------------------
// Read API
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::get(const char* path, bool& outValue)
{
    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getBool(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Boolean, path);
        return mapped;
    }

    outValue = m_fbData.boolData();
    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::Boolean,
              path);
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::get(const char* path, int& outValue)
{
    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getInt(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Integer, path);
        return mapped;
    }

    outValue = m_fbData.intData();
    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::Integer,
              path);
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::get(const char* path, float& outValue)
{
    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getFloat(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Float, path);
        return mapped;
    }

    outValue = m_fbData.floatData();
    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::Float,
              path);
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::get(const char* path, double& outValue)
{
    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getDouble(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Double, path);
        return mapped;
    }

    outValue = m_fbData.doubleData();
    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::Double,
              path);
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::get(const char* path,
                                     char*       outBuffer,
                                     size_t      bufferLen)
{
    if (outBuffer == nullptr || bufferLen == 0)
    {
        return FirebaseResult::InvalidValue;
    }

    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getString(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::String, path);
        return mapped;
    }

    strncpy(outBuffer, m_fbData.stringData().c_str(), bufferLen - 1);
    outBuffer[bufferLen - 1] = '\0';

    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::String,
              path);
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::get(const char* path, JsonDocument& outDoc)
{
    const FirebaseResult pre = validateReady();
    if (pre != FirebaseResult::Success) { return pre; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (!Firebase.RTDB.getJSON(&m_fbData, path))
    {
        const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
        fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Json, path);
        return mapped;
    }

    // Deserialise the Firebase JSON string into the caller's ArduinoJson doc.
    const DeserializationError err =
        deserializeJson(outDoc, m_fbData.jsonString().c_str());

    if (err)
    {
        fireEvent(FirebaseEvent::Error,
                  FirebaseResult::InvalidType,
                  FirebaseDataType::Json,
                  path);
        return FirebaseResult::InvalidType;
    }

    fireEvent(FirebaseEvent::ReadCompleted,
              FirebaseResult::Success,
              FirebaseDataType::Json,
              path);
    return FirebaseResult::Success;
}

// ---------------------------------------------------------------------------
// Write API
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::set(const char* path, bool value)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (m_state == FirebaseState::Ready)
    {
        if (!Firebase.RTDB.setBool(&m_fbData, path, value))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Boolean, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Boolean,
                  path);
        return FirebaseResult::Success;
    }

    // Not ready — queue for later delivery.
    QueueEntry entry{};
    strncpy(entry.path, path, FIREBASE_MAX_PATH_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1] = '\0';
    entry.opType  = QueuedOpType::WriteBool;
    entry.boolVal = value;

    return enqueueWrite(entry);
}

FirebaseResult FirebaseManager::set(const char* path, int value)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (m_state == FirebaseState::Ready)
    {
        if (!Firebase.RTDB.setInt(&m_fbData, path, value))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Integer, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Integer,
                  path);
        return FirebaseResult::Success;
    }

    QueueEntry entry{};
    strncpy(entry.path, path, FIREBASE_MAX_PATH_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1] = '\0';
    entry.opType = QueuedOpType::WriteInt;
    entry.intVal = value;

    return enqueueWrite(entry);
}

FirebaseResult FirebaseManager::set(const char* path, float value)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (m_state == FirebaseState::Ready)
    {
        if (!Firebase.RTDB.setFloat(&m_fbData, path, value))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Float, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Float,
                  path);
        return FirebaseResult::Success;
    }

    QueueEntry entry{};
    strncpy(entry.path, path, FIREBASE_MAX_PATH_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1] = '\0';
    entry.opType   = QueuedOpType::WriteFloat;
    entry.floatVal = value;

    return enqueueWrite(entry);
}

FirebaseResult FirebaseManager::set(const char* path, double value)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (m_state == FirebaseState::Ready)
    {
        if (!Firebase.RTDB.setDouble(&m_fbData, path, value))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Double, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Double,
                  path);
        return FirebaseResult::Success;
    }

    QueueEntry entry{};
    strncpy(entry.path, path, FIREBASE_MAX_PATH_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1] = '\0';
    entry.opType = QueuedOpType::WriteDouble;
    entry.doubleVal = value;

    return enqueueWrite(entry);
}

FirebaseResult FirebaseManager::set(const char* path, const char* value)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }
    if (value == nullptr) { return FirebaseResult::InvalidValue; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (m_state == FirebaseState::Ready)
    {
        if (!Firebase.RTDB.setString(&m_fbData, path, value))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::String, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::String,
                  path);
        return FirebaseResult::Success;
    }

    QueueEntry entry{};
    strncpy(entry.path,      path,  FIREBASE_MAX_PATH_LEN - 1);
    strncpy(entry.stringVal, value, FIREBASE_MAX_VALUE_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1]      = '\0';
    entry.stringVal[FIREBASE_MAX_VALUE_LEN - 1] = '\0';
    entry.opType = QueuedOpType::WriteString;

    return enqueueWrite(entry);
}

FirebaseResult FirebaseManager::set(const char* path, const JsonDocument& doc)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    // Serialise the ArduinoJson document to a stack-allocated string buffer.
    char jsonBuf[FIREBASE_MAX_VALUE_LEN];
    const size_t written = serializeJson(doc, jsonBuf, sizeof(jsonBuf));
    if (written == 0 || written >= sizeof(jsonBuf))
    {
        return FirebaseResult::InvalidValue;
    }

    if (m_state == FirebaseState::Ready)
    {
        FirebaseJson fbJson;
        fbJson.setJsonData(jsonBuf);   // load from string into FirebaseJson

        if (!Firebase.RTDB.setJSON(&m_fbData, path, &fbJson))
        {
            const FirebaseResult mapped = mapHttpCode(m_fbData.httpCode());
            fireEvent(FirebaseEvent::Error, mapped, FirebaseDataType::Json, path);
            return mapped;
        }
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Json,
                  path);
        return FirebaseResult::Success;
    }

    QueueEntry entry{};
    strncpy(entry.path,      path,    FIREBASE_MAX_PATH_LEN - 1);
    strncpy(entry.stringVal, jsonBuf, FIREBASE_MAX_VALUE_LEN - 1);
    entry.path[FIREBASE_MAX_PATH_LEN - 1]      = '\0';
    entry.stringVal[FIREBASE_MAX_VALUE_LEN - 1] = '\0';
    entry.opType = QueuedOpType::WriteJson;

    return enqueueWrite(entry);
}

// ---------------------------------------------------------------------------
// Streaming
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::subscribe(const char* path)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const FirebaseResult pathCheck = validatePath(path);
    if (pathCheck != FirebaseResult::Success) { return pathCheck; }

    if (findSubscriptionIndex(path) >= 0)
    {
        return FirebaseResult::SubscriptionExists;
    }

    if (m_subscriptionCount >= FIREBASE_MAX_SUBSCRIPTIONS)
    {
        return FirebaseResult::QueueFull;
    }

    strncpy(m_subscriptions[m_subscriptionCount], path, FIREBASE_MAX_PATH_LEN - 1);
    m_subscriptions[m_subscriptionCount][FIREBASE_MAX_PATH_LEN - 1] = '\0';
    m_subscriptionCount++;

    // If this is the first subscription and Firebase is ready, start streaming
    // immediately; otherwise it will be activated in tickStream().
    if (m_subscriptionCount == 1 && m_state == FirebaseState::Ready)
    {
        m_activeStreamIndex = 0;
        beginStream();
    }

    fireEvent(FirebaseEvent::SubscriptionAdded,
              FirebaseResult::Success,
              FirebaseDataType::Unknown,
              path);

    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::unsubscribe(const char* path)
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    const int index = findSubscriptionIndex(path);
    if (index < 0)
    {
        return FirebaseResult::SubscriptionNotFound;
    }

    // Compact the subscription array by shifting entries down.
    const uint8_t idx = static_cast<uint8_t>(index);
    for (uint8_t i = idx; i < m_subscriptionCount - 1; i++)
    {
        strncpy(m_subscriptions[i],
                m_subscriptions[i + 1],
                FIREBASE_MAX_PATH_LEN);
    }
    m_subscriptions[m_subscriptionCount - 1][0] = '\0';
    m_subscriptionCount--;

    // If we removed the currently streamed path, restart the stream engine.
    if (idx <= m_activeStreamIndex && m_subscriptionCount > 0)
    {
        if (m_activeStreamIndex > 0)
        {
            m_activeStreamIndex--;
        }
        m_streamBegun = false;
    }

    if (m_subscriptionCount == 0)
    {
        Firebase.RTDB.endStream(&m_fbStream);
        m_streamBegun = false;
    }

    fireEvent(FirebaseEvent::SubscriptionRemoved,
              FirebaseResult::Success,
              FirebaseDataType::Unknown,
              path);

    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::unsubscribeAll()
{
    if (!m_enabled)    { return FirebaseResult::Disabled; }
    if (!m_configured) { return FirebaseResult::NotConfigured; }

    Firebase.RTDB.endStream(&m_fbStream);
    m_streamBegun = false;

    for (uint8_t i = 0; i < m_subscriptionCount; i++)
    {
        m_subscriptions[i][0] = '\0';
    }
    m_subscriptionCount = 0;
    m_activeStreamIndex = 0;

    return FirebaseResult::Success;
}

// ---------------------------------------------------------------------------
// Callback
// ---------------------------------------------------------------------------

void FirebaseManager::onEvent(EventCallback callback, void* context)
{
    m_eventCallback = callback;
    m_eventContext  = context;
}

// ---------------------------------------------------------------------------
// Private — connection state machine
// ---------------------------------------------------------------------------

void FirebaseManager::tickConnection(uint32_t nowMs)
{
    switch (m_state)
    {
        case FirebaseState::Connecting:
        {
            if (!isWiFiConnected())
            {
                // WiFi not yet available; remain in Connecting.
                return;
            }

            // Honour the exponential backoff delay between attempts.
            if (m_retryCount > 0 && (nowMs - m_retryTimerMs) < m_retryDelayMs)
            {
                return;
            }

            if (!m_fbBegun)
            {
                // First authentication: run Firebase.begin() in the storm-
                // protected task (see THREADING EXCEPTION in the header).
                if (!m_initTaskLaunched)
                {
                    if (launchInitTask())
                    {
                        m_connectStartMs = nowMs;
                        transitionTo(FirebaseState::Authenticating);
                    }
                    else
                    {
                        // Could not spawn the task (heap pressure) — back off.
                        scheduleRetry();
                    }
                }
            }
            else
            {
                // Reconnect: credentials are already proven valid, so the cheap
                // Firebase.ready() polling path on the main thread cannot storm.
                m_connectStartMs   = nowMs;
                m_lastReadyCheckMs = 0;   // allow an immediate first check
                transitionTo(FirebaseState::Authenticating);
            }
            break;
        }

        case FirebaseState::Authenticating:
        {
            if (!isWiFiConnected())
            {
                if (m_initTaskLaunched)
                {
                    deleteInitTask();
                }
                transitionTo(FirebaseState::Disconnected);
                fireEvent(FirebaseEvent::Disconnected);
                return;
            }

            if (!m_fbBegun)
            {
                tickFirstAuth(nowMs);
            }
            else
            {
                tickReconnectAuth(nowMs);
            }
            break;
        }

        case FirebaseState::Ready:
            // Heartbeat tick handles Ready state monitoring.
            break;

        case FirebaseState::Disconnected:
        {
            if (m_manualDisconnect)
            {
                return;   // application called disconnect(); do not auto-reconnect
            }

            if (!isWiFiConnected())
            {
                return;   // wait for WiFi to return
            }

            // WiFi is back — start a new Firebase session.
            scheduleRetry();
            if (m_state != FirebaseState::Error)
            {
                transitionTo(FirebaseState::Connecting);
                fireEvent(FirebaseEvent::RetryStarted);
            }
            break;
        }

        default:
            // NotConfigured / Configured / Disabled / Error — no auto-driving.
            // Error is terminal: recovery requires reconnect() or a reboot.
            break;
    }
}

void FirebaseManager::tickFirstAuth(uint32_t nowMs)
{
    // 1) A token error means Firebase.begin() is looping on a failed auth.
    //    Abort the task immediately so it cannot storm the auth endpoint, then
    //    decide recovery based on the error class.
    if (s_tokenError)
    {
        const TokenErrorClass cls = classifyTokenError();
        s_tokenError = false;
        deleteInitTask();

        switch (cls)
        {
            case TokenErrorClass::Credential:
                // Wrong key/email/password or disabled provider — retrying is
                // pointless and would re-storm.  Terminal until reconnect().
                transitionTo(FirebaseState::Error);
                fireEvent(FirebaseEvent::Error, FirebaseResult::AuthenticationFailed);
                return;

            case TokenErrorClass::Quota:
                // TOO_MANY_ATTEMPTS — backing off here is not enough; any retry
                // risks deepening the ban.  Terminal until reconnect() after the
                // cooldown (or a reboot).
                transitionTo(FirebaseState::Error);
                fireEvent(FirebaseEvent::Error, FirebaseResult::RateLimited);
                return;

            default:
                // Transient network/TCP error — bounded exponential backoff.
                scheduleRetry();
                if (m_state != FirebaseState::Error)
                {
                    transitionTo(FirebaseState::Connecting);
                }
                return;
        }
    }

    // 2) Task finished.
    const InitOutcome outcome = m_initOutcome;

    if (outcome == InitOutcome::Success)
    {
        // Firebase.begin() returned → authentication succeeded.
        deleteInitTask();
        m_fbBegun = true;

        // Single confirmation read (task is gone; this cannot race).
        if (Firebase.ready())
        {
            enterReady(nowMs);
        }
        else
        {
            // Begin succeeded but token not yet flagged ready; the cheap
            // reconnect path will confirm it on subsequent ticks.
            m_lastReadyCheckMs = 0;
        }
        return;
    }

    if (outcome == InitOutcome::NtpFailed)
    {
        // No wall-clock time — treat as transient and back off.
        deleteInitTask();
        scheduleRetry();
        if (m_state != FirebaseState::Error)
        {
            transitionTo(FirebaseState::Connecting);
        }
        return;
    }

    // 3) Still pending — backstop timeout in case the task wedged inside TLS.
    if ((nowMs - m_connectStartMs) >= AUTH_TIMEOUT_MS)
    {
        deleteInitTask();
        scheduleRetry();
        if (m_state != FirebaseState::Error)
        {
            transitionTo(FirebaseState::Connecting);
        }
    }
}

void FirebaseManager::tickReconnectAuth(uint32_t nowMs)
{
    // Throttle Firebase.ready() — it can block briefly during token refresh.
    if ((nowMs - m_lastReadyCheckMs) < READY_CHECK_INTERVAL_MS)
    {
        return;
    }
    m_lastReadyCheckMs = nowMs;

    if (Firebase.ready())
    {
        enterReady(nowMs);
        return;
    }

    if ((nowMs - m_connectStartMs) >= AUTH_TIMEOUT_MS)
    {
        scheduleRetry();
        if (m_state != FirebaseState::Error)
        {
            transitionTo(FirebaseState::Connecting);
        }
    }
}

void FirebaseManager::enterReady(uint32_t nowMs)
{
    // Capture before resetRetry() zeroes m_retryCount.
    const bool wasRetrying = (m_retryCount > 0);

    resetRetry();
    m_lastReadyMs = nowMs;

    transitionTo(FirebaseState::Connected);
    fireEvent(FirebaseEvent::Connected);
    fireEvent(FirebaseEvent::Authenticated);

    transitionTo(FirebaseState::Ready);
    fireEvent(FirebaseEvent::Ready);

    if (wasRetrying)
    {
        fireEvent(FirebaseEvent::RetryCompleted);
    }
}

void FirebaseManager::tickHeartbeat(uint32_t nowMs)
{
    if (m_state != FirebaseState::Ready)
    {
        return;
    }

    if ((nowMs - m_lastHeartbeatMs) < HEARTBEAT_INTERVAL_MS)
    {
        return;
    }
    m_lastHeartbeatMs = nowMs;

    if (!isWiFiConnected())
    {
        transitionTo(FirebaseState::Disconnected);
        m_streamBegun = false;
        fireEvent(FirebaseEvent::Disconnected);
        return;
    }

    if (!Firebase.ready())
    {
        // Firebase library not ready — possibly token refresh in progress.
        // Allow a short grace period before acting to avoid false alarms
        // caused by the brief moment when the library refreshes the token.
        //
        // NOTE: m_fbBegun is intentionally left true.  Recovery uses the cheap
        // Firebase.ready() polling path (tickReconnectAuth), never a new auth
        // task — the credentials are already proven valid, so there is no storm
        // risk and no need to repeat the heavyweight first-auth path.
        if ((nowMs - m_lastReadyMs) > READY_GRACE_PERIOD_MS)
        {
            m_streamBegun = false;
            scheduleRetry();
            if (m_state != FirebaseState::Error)
            {
                transitionTo(FirebaseState::Connecting);
                fireEvent(FirebaseEvent::Disconnected);
            }
        }
        return;
    }

    m_lastReadyMs = nowMs;
}

void FirebaseManager::tickStream(uint32_t nowMs)
{
    if (m_state != FirebaseState::Ready || m_subscriptionCount == 0)
    {
        return;
    }

    if (!m_streamBegun)
    {
        beginStream();
        return;
    }

    // Throttle readStream() — calling it every loop() iteration can block and
    // starve other modules.  50 ms gives adequate stream responsiveness (20 Hz).
    if ((nowMs - m_lastStreamReadMs) < STREAM_READ_INTERVAL_MS)
    {
        return;
    }
    m_lastStreamReadMs = nowMs;

    if (!Firebase.RTDB.readStream(&m_fbStream))
    {
        // Transient stream error; stream engine will reconnect automatically.
        return;
    }

    if (m_fbStream.streamAvailable())
    {
        fireEvent(FirebaseEvent::StreamUpdated,
                  FirebaseResult::Success,
                  FirebaseDataType::Unknown,
                  m_subscriptions[m_activeStreamIndex]);

        // Round-robin through all subscribed paths so each receives equal
        // stream coverage.  advanceStreamIndex() sets m_streamBegun = false,
        // causing beginStream() to reconnect to the next path next tick.
        if (m_subscriptionCount > 1)
        {
            advanceStreamIndex();
        }
    }
}

void FirebaseManager::tickQueue(uint32_t nowMs)
{
    if (m_state != FirebaseState::Ready)
    {
        // Left Ready mid-flush; reset so QueueStarted fires again when Ready returns.
        m_queueFlushStarted = false;
        return;
    }

    if (m_queueCount == 0)
    {
        return;
    }

    // Process one queue entry per tick interval to keep loop() short.
    if ((nowMs - m_lastQueueTickMs) < QUEUE_TICK_INTERVAL_MS)
    {
        return;
    }
    m_lastQueueTickMs = nowMs;

    if (!m_queueFlushStarted)
    {
        m_queueFlushStarted = true;
        fireEvent(FirebaseEvent::QueueStarted);
    }

    QueueEntry entry{};
    if (!dequeueWrite(entry))
    {
        return;
    }

    // Failed entries are dropped — Phase 1 has no persistent retry storage.
    if (!executeQueueEntry(entry))
    {
        fireEvent(FirebaseEvent::QueueOperationFailed,
                  mapHttpCode(m_fbData.httpCode()),
                  FirebaseDataType::Unknown,
                  entry.path);
    }

    if (m_queueCount == 0)
    {
        m_queueFlushStarted = false;
        fireEvent(FirebaseEvent::QueueFlushed);
        fireEvent(FirebaseEvent::QueueEmpty);
    }
}

// ---------------------------------------------------------------------------
// Private — async auth task  (see THREADING EXCEPTION in the header)
// ---------------------------------------------------------------------------

bool FirebaseManager::launchInitTask()
{
    // Bind the singleton and reset the shared error channel.
    s_instance       = this;
    s_tokenError     = false;
    s_tokenErrCode   = 0;
    s_tokenErrMsg[0] = '\0';
    m_initOutcome    = InitOutcome::Pending;
    m_initTaskHandle = nullptr;

    // Load credentials and tuning into the library structs (once per attempt).
    m_fbConfig.api_key      = m_apiKey;
    m_fbConfig.database_url = m_databaseUrl;
    m_fbAuth.user.email     = m_userEmail;
    m_fbAuth.user.password  = m_userPassword;

    // A token-status callback lets us learn the *reason* an auth failed and
    // abort the storming begin() loop after a single attempt.  The short
    // tokenGenerationError interval makes the first error callback fire fast.
    m_fbConfig.token_status_callback        = tokenStatusCallbackStatic;
    m_fbConfig.timeout.serverResponse       = SERVER_RESPONSE_TIMEOUT_MS;
    m_fbConfig.timeout.socketConnection     = SOCKET_CONNECT_TIMEOUT_MS;
    m_fbConfig.timeout.tokenGenerationError = TOKEN_GENERATION_ERROR_MS;
    m_fbConfig.timeout.ntpServerRequest     = NTP_SYNC_TIMEOUT_MS;

    const BaseType_t created = xTaskCreatePinnedToCore(
        initTaskTrampoline,
        "fb_init",
        INIT_TASK_STACK_BYTES,
        this,
        1,                  // priority just above idle
        &m_initTaskHandle,
        INIT_TASK_CORE);

    if (created != pdPASS)
    {
        m_initTaskHandle = nullptr;
        return false;
    }

    m_initTaskLaunched = true;
    return true;
}

void FirebaseManager::deleteInitTask()
{
    // The main loop is the SOLE deleter (the task self-parks rather than
    // self-deleting), so this is always a single, safe delete.
    if (m_initTaskHandle != nullptr)
    {
        vTaskDelete(m_initTaskHandle);
        m_initTaskHandle = nullptr;
    }
    m_initTaskLaunched = false;
    m_initOutcome      = InitOutcome::Pending;
}

void FirebaseManager::initTaskTrampoline(void* param)
{
    static_cast<FirebaseManager*>(param)->runInitTask();
}

void FirebaseManager::runInitTask()
{
    if (!syncTimeWithTimeout(NTP_SYNC_TIMEOUT_MS))
    {
        m_initOutcome = InitOutcome::NtpFailed;
    }
    else
    {
        // Returns ONLY on auth success.  On any token error the library loops
        // forever; the main loop sees s_tokenError and deletes this task.
        Firebase.begin(&m_fbConfig, &m_fbAuth);
        Firebase.reconnectNetwork(true);

        // Shrink the RTDB client TLS buffers to keep heap usage low.
        m_fbData.setBSSLBufferSize(BSSL_RX_BYTES, BSSL_TX_BYTES);
        m_fbStream.setBSSLBufferSize(BSSL_RX_BYTES, BSSL_TX_BYTES);

        m_initOutcome = InitOutcome::Success;
    }

    // Park forever.  The main loop owns deletion of this task to guarantee a
    // single deleter and avoid any double-free race.
    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

bool FirebaseManager::syncTimeWithTimeout(uint32_t timeoutMs)
{
    // Firebase TLS cert validation and the token flow both require real time.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");

    const uint32_t start = millis();
    time_t         now   = time(nullptr);

    while (static_cast<uint32_t>(now) < MIN_VALID_EPOCH)
    {
        if ((millis() - start) >= timeoutMs)
        {
            return false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
        now = time(nullptr);
    }

    return true;
}

FirebaseManager::TokenErrorClass FirebaseManager::classifyTokenError() const
{
    const char* m = s_tokenErrMsg;

    // Rate-limit — the most important to detect; never auto-retry.
    if (strstr(m, "TOO_MANY_ATTEMPTS") != nullptr)
    {
        return TokenErrorClass::Quota;
    }

    // Credential / configuration problems — terminal, retrying cannot help.
    if (strstr(m, "INVALID_PASSWORD")        != nullptr ||
        strstr(m, "EMAIL_NOT_FOUND")         != nullptr ||
        strstr(m, "INVALID_LOGIN")           != nullptr ||
        strstr(m, "INVALID_EMAIL")           != nullptr ||
        strstr(m, "USER_DISABLED")           != nullptr ||
        strstr(m, "MISSING_PASSWORD")        != nullptr ||
        strstr(m, "MISSING_EMAIL")           != nullptr ||
        strstr(m, "OPERATION_NOT_ALLOWED")   != nullptr ||
        strstr(m, "PASSWORD_LOGIN_DISABLED") != nullptr ||
        strstr(m, "API key not valid")       != nullptr ||
        strstr(m, "CREDENTIAL")              != nullptr)
    {
        return TokenErrorClass::Credential;
    }

    // Anything else (TCP/connection lost, response timeout) is transient.
    return TokenErrorClass::Transient;
}

void FirebaseManager::tokenStatusCallbackStatic(TokenInfo info)
{
    if (info.status != token_status_error)
    {
        return;
    }

    s_tokenErrCode = info.error.code;
    strncpy(s_tokenErrMsg, info.error.message.c_str(), TOKEN_ERR_MSG_LEN - 1);
    s_tokenErrMsg[TOKEN_ERR_MSG_LEN - 1] = '\0';

    // Raise the flag LAST so a main-loop reader that observes it also sees a
    // fully-written message buffer.
    s_tokenError = true;
}

void FirebaseManager::transitionTo(FirebaseState newState)
{
    m_state = newState;
}

void FirebaseManager::scheduleRetry()
{
    if (m_retryCount >= FIREBASE_MAX_RETRY_COUNT)
    {
        transitionTo(FirebaseState::Error);
        fireEvent(FirebaseEvent::RetryLimitReached,
                  FirebaseResult::Timeout);
        return;
    }

    m_retryTimerMs = millis();
    m_retryDelayMs = (m_retryCount == 0)
                     ? RETRY_BASE_DELAY_MS
                     : min(m_retryDelayMs * 2, RETRY_MAX_DELAY_MS);
    m_retryCount++;
}

void FirebaseManager::resetRetry()
{
    m_retryCount   = 0;
    m_retryDelayMs = RETRY_BASE_DELAY_MS;
    m_retryTimerMs = 0;
}

// ---------------------------------------------------------------------------
// Private — operations
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::validateReady() const
{
    if (!m_enabled)             { return FirebaseResult::Disabled; }
    if (!m_configured)          { return FirebaseResult::NotConfigured; }
    if (m_state != FirebaseState::Ready) { return FirebaseResult::NotConnected; }
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::validatePath(const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return FirebaseResult::InvalidPath;
    }
    return FirebaseResult::Success;
}

FirebaseResult FirebaseManager::mapHttpCode(int httpCode)
{
    switch (httpCode)
    {
        case 200: case 204: return FirebaseResult::Success;
        case 401:           return FirebaseResult::AuthenticationFailed;
        case 403:           return FirebaseResult::PermissionDenied;
        case 404:           return FirebaseResult::InvalidPath;
        case 408: case 504: return FirebaseResult::Timeout;
        case 429:           return FirebaseResult::RateLimited;
        default:            return FirebaseResult::UnknownError;
    }
}

bool FirebaseManager::executeQueueEntry(const QueueEntry& entry)
{
    bool ok = false;

    switch (entry.opType)
    {
        case QueuedOpType::WriteBool:
            ok = Firebase.RTDB.setBool(&m_fbData, entry.path, entry.boolVal);
            break;

        case QueuedOpType::WriteInt:
            ok = Firebase.RTDB.setInt(&m_fbData, entry.path, entry.intVal);
            break;

        case QueuedOpType::WriteFloat:
            ok = Firebase.RTDB.setFloat(&m_fbData, entry.path, entry.floatVal);
            break;

        case QueuedOpType::WriteDouble:
            ok = Firebase.RTDB.setDouble(&m_fbData, entry.path, entry.doubleVal);
            break;

        case QueuedOpType::WriteString:
            ok = Firebase.RTDB.setString(&m_fbData, entry.path, entry.stringVal);
            break;

        case QueuedOpType::WriteJson:
        {
            FirebaseJson fbJson;
            fbJson.setJsonData(entry.stringVal);
            ok = Firebase.RTDB.setJSON(&m_fbData, entry.path, &fbJson);
            break;
        }

        default:
            return true;   // unknown op type — drop silently, not a retry candidate
    }

    if (ok)
    {
        fireEvent(FirebaseEvent::WriteCompleted,
                  FirebaseResult::Success,
                  FirebaseDataType::Unknown,
                  entry.path);
    }
    // Failure event is fired by tickQueue as QueueOperationFailed.

    return ok;
}

// ---------------------------------------------------------------------------
// Private — queue
// ---------------------------------------------------------------------------

FirebaseResult FirebaseManager::enqueueWrite(const QueueEntry& entry)
{
    if (m_queueCount >= FIREBASE_MAX_QUEUE_OPERATIONS)
    {
        fireEvent(FirebaseEvent::QueueFull,
                  FirebaseResult::QueueFull,
                  FirebaseDataType::Unknown,
                  entry.path);
        return FirebaseResult::QueueFull;
    }

    m_queue[m_queueTail] = entry;
    m_queueTail = static_cast<uint8_t>((m_queueTail + 1) % FIREBASE_MAX_QUEUE_OPERATIONS);
    m_queueCount++;

    return FirebaseResult::Queued;
}

bool FirebaseManager::dequeueWrite(QueueEntry& outEntry)
{
    if (m_queueCount == 0)
    {
        return false;
    }

    outEntry   = m_queue[m_queueHead];
    m_queueHead = static_cast<uint8_t>((m_queueHead + 1) % FIREBASE_MAX_QUEUE_OPERATIONS);
    m_queueCount--;

    return true;
}

// ---------------------------------------------------------------------------
// Private — subscriptions
// ---------------------------------------------------------------------------

int FirebaseManager::findSubscriptionIndex(const char* path) const
{
    for (uint8_t i = 0; i < m_subscriptionCount; i++)
    {
        if (strncmp(m_subscriptions[i], path, FIREBASE_MAX_PATH_LEN) == 0)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FirebaseManager::beginStream()
{
    if (m_subscriptionCount == 0 || m_activeStreamIndex >= m_subscriptionCount)
    {
        return;
    }

    Firebase.RTDB.beginStream(&m_fbStream,
                               m_subscriptions[m_activeStreamIndex]);
    m_streamBegun = true;
}

void FirebaseManager::advanceStreamIndex()
{
    if (m_subscriptionCount == 0)
    {
        return;
    }
    m_activeStreamIndex =
        static_cast<uint8_t>((m_activeStreamIndex + 1) % m_subscriptionCount);
    m_streamBegun = false;
}

// ---------------------------------------------------------------------------
// Private — events
// ---------------------------------------------------------------------------

void FirebaseManager::fireEvent(FirebaseEvent    event,
                                 FirebaseResult   result,
                                 FirebaseDataType type,
                                 const char*      path)
{
    if (m_eventCallback != nullptr)
    {
        m_eventCallback(event, result, type, path, m_eventContext);
    }
}

// ---------------------------------------------------------------------------
// Private — WiFi
// ---------------------------------------------------------------------------

bool FirebaseManager::isWiFiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
