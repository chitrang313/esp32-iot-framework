# FirebaseManager - instruction-part2.md

# Part 2 - Authentication & Connection Management

---

# Objective

This part defines how FirebaseManager establishes, maintains and restores a Firebase connection.

The application should never perform authentication or reconnection manually.

FirebaseManager owns the complete connection lifecycle.

---

# Connection Philosophy

FirebaseManager must be autonomous.

Application code should never need to write

```cpp
if(!firebase.isConnected())
{
    firebase.connect();
}
```

Instead

```cpp
firebase.begin();
```

followed by

```cpp
firebase.loop();
```

must automatically manage everything.

---

# Dependency

FirebaseManager depends on

IoTWiFiManager

FirebaseManager must never attempt to connect while WiFi is unavailable.

Example

```text
WiFi

↓

Connected

↓

Firebase Connect

↓

Authenticate

↓

Ready
```

If WiFi disconnects

```text
Firebase Ready

↓

WiFi Lost

↓

Firebase Disconnected

↓

Wait

↓

WiFi Restored

↓

Reconnect

↓

Authenticate

↓

Ready
```

Application code should never perform this sequence.

---

# Authentication

Support

- Email / Password
- Anonymous Authentication (Future)

Phase 1 implements

Email / Password

---

# Configuration

Authentication credentials are provided once.

Example

```cpp
firebase.configure(

    API_KEY,

    DATABASE_URL,

    USER_EMAIL,

    USER_PASSWORD

);
```

Credentials are stored internally.

Authentication starts only after begin().

---

# Authentication Rules

Authentication should

- occur automatically
- retry automatically
- refresh tokens automatically
- never block

---

# Token Refresh

Firebase tokens expire.

FirebaseManager must automatically refresh tokens.

Application code must never call

```cpp
refreshToken()
```

The application should not know token expiration exists.

---

# Ready State

FirebaseManager is considered Ready only when

- WiFi Connected
- Authentication Successful
- Token Valid
- Database Available

Until then

```cpp
firebase.isReady()
```

returns false.

---

# Automatic Connection

begin()

↓

Waiting for WiFi

↓

Connecting

↓

Authenticating

↓

Ready

No application code required.

---

# Automatic Reconnection

If Firebase disconnects

↓

Retry automatically.

If WiFi disconnects

↓

Wait for WiFi.

↓

Reconnect automatically.

Never require

```cpp
firebase.begin();
```

again.

---

# Retry Strategy

Do not reconnect continuously.

Retry intelligently.

Recommended

```text
Attempt 1

↓

Wait

↓

Attempt 2

↓

Wait

↓

Attempt 3

↓

Increase delay

↓

Attempt 4

↓

Increase delay

↓

...
```

The retry engine must never consume excessive CPU time.

---

# Connection Timeout

Every connection attempt should have a timeout.

Never wait forever.

When timeout occurs

- update state
- publish event
- schedule retry

---

# Authentication Failure

Examples

Wrong password

↓

AuthenticationFailed

↓

Retry only if credentials change or application requests reconfiguration.

Do not continuously retry invalid credentials.

---

# Connection State Machine

Internal state transitions

```text
NotConfigured

↓

Configured

↓

WaitingForWiFi

↓

Connecting

↓

Authenticating

↓

Connected

↓

Ready
```

On WiFi loss

```text
Ready

↓

Disconnected

↓

WaitingForWiFi
```

On Firebase failure

```text
Ready

↓

Connecting

↓

Authenticating

↓

Ready
```

---

# Heartbeat

FirebaseManager should periodically verify that the connection is still alive.

Implementation details are left to the coding agent.

The application should never perform heartbeat checks.

---

# WiFi Monitoring

FirebaseManager should monitor WiFi state.

When WiFi changes

↓

FirebaseManager reacts automatically.

Application code should not coordinate this.

---

# Connection Events

Publish events when

- Connected
- Disconnected
- Authentication Successful
- Authentication Failed
- Ready
- Retry Started
- Retry Completed
- Timeout
- Error

Events are informational only.

Application decides what to do.

---

# Event Rules

Never call application callbacks from interrupt context.

Callbacks should execute from normal loop() execution.

Callbacks must never block FirebaseManager.

---

# State Ownership

Only FirebaseManager may modify

- Authentication State
- Connection State
- Retry State

Application code is read-only.

---

# Internal Objects

Internally maintain

- Authentication Status
- Connection Status
- Retry Counter
- Retry Timer
- Token Status
- WiFi Status
- Current FirebaseState

These are private.

Never expose them publicly.

---

# Validation

Before every Firebase operation verify

- Module enabled
- Configured
- WiFi Connected
- Authenticated
- Ready

If any check fails

Return the appropriate

```text
FirebaseResult
```

Never crash.

Never block.

Never silently fail.

---

# Future Compatibility

Architecture should support

- Multiple authentication methods
- Service accounts
- Firestore
- Multiple Firebase projects

without changing the public API.

---

# Final Goal

The application should simply write

```cpp
firebase.begin();

firebase.loop();
```

FirebaseManager should automatically

- Wait for WiFi
- Connect
- Authenticate
- Refresh Tokens
- Detect Failures
- Retry Connections
- Recover Automatically

The application should only observe

```cpp
firebase.isReady();

firebase.getState();

firebase.onEvent(...);
```

Everything else belongs inside FirebaseManager.
