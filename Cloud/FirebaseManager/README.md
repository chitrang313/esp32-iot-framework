# FirebaseManager

Reusable Firebase Realtime Database manager for ESP32.

Supports:

- Firebase Authentication
- Automatic Connection Management
- Automatic Reconnection
- Token Refresh
- Read Operations
- Write Operations
- Realtime Streaming
- Callback Events
- Offline Queue (Future)
- Non-blocking Operation

---

# Installation

Copy the module into your project.

```text
Cloud/
└── FirebaseManager/
    ├── FirebaseManager.h
    ├── FirebaseManager.cpp
    ├── README.md
    └── instruction.md
```

Include the header.

```cpp
#include "Cloud/FirebaseManager/FirebaseManager.h"
```

---

# Create Object

```cpp
FirebaseManager firebase;
```

---

# Configure Firebase

Configure Firebase before calling `begin()`.

```cpp
firebase.configure(
    API_KEY,
    DATABASE_URL,
    USER_EMAIL,
    USER_PASSWORD
);
```

---

# Setup

```cpp
void setup()
{
    firebase.begin();
}
```

---

# Loop

Always call `loop()`.

```cpp
void loop()
{
    firebase.loop();
}
```

---

# Enable / Disable

```cpp
firebase.enable();

firebase.disable();

firebase.isEnabled();
```

---

# Connection

Connect.

```cpp
firebase.connect();
```

Disconnect.

```cpp
firebase.disconnect();
```

Reconnect.

```cpp
firebase.reconnect();
```

---

# Connection Status

Check connection.

```cpp
firebase.isConnected();
```

Check authentication.

```cpp
firebase.isAuthenticated();
```

Check ready state.

```cpp
firebase.isReady();
```

Get current state.

```cpp
FirebaseState state =
    firebase.getState();
```

---

# Read Data

Read Boolean.

```cpp
firebase.getBool(
    FirebasePath::KitchenLight
);
```

Read Integer.

```cpp
firebase.getInt(
    FirebasePath::Counter
);
```

Read Float.

```cpp
firebase.getFloat(
    FirebasePath::Temperature
);
```

Read Double.

```cpp
firebase.getDouble(
    FirebasePath::Voltage
);
```

Read String.

```cpp
firebase.getString(
    FirebasePath::DeviceName
);
```

Read JSON.

```cpp
firebase.getJson(
    FirebasePath::Configuration
);
```

---

# Write Data

Write Boolean.

```cpp
firebase.setBool(
    FirebasePath::KitchenLight,
    true
);
```

Write Integer.

```cpp
firebase.setInt(
    FirebasePath::Counter,
    25
);
```

Write Float.

```cpp
firebase.setFloat(
    FirebasePath::Temperature,
    24.6f
);
```

Write Double.

```cpp
firebase.setDouble(
    FirebasePath::Voltage,
    230.45
);
```

Write String.

```cpp
firebase.setString(
    FirebasePath::DeviceName,
    "ESP32"
);
```

Write JSON.

```cpp
firebase.setJson(
    FirebasePath::Configuration,
    json
);
```

---

# Realtime Streaming

Subscribe.

```cpp
firebase.subscribe(
    FirebasePath::KitchenLight
);
```

Unsubscribe.

```cpp
firebase.unsubscribe(
    FirebasePath::KitchenLight
);
```

---

# Events

Register a single event callback.

```cpp
firebase.onEvent(
    onFirebaseEvent
);
```

Example.

```cpp
void onFirebaseEvent(
    FirebaseEvent event,
    const char* path,
    void* context
)
{
    // Application decides what to do
}
```

---

# Return Result

Every read and write operation returns a `FirebaseResult`.

```cpp
FirebaseResult result =
    firebase.setBool(
        FirebasePath::KitchenLight,
        true
    );

if(result == FirebaseResult::Success)
{
    // Write completed
}
```

---

# Enumerations

## FirebaseState

```cpp
Disconnected

Connecting

Authenticating

Connected

Ready

Error

Disabled
```

---

## FirebaseResult

```cpp
Success

Queued

InvalidPath

InvalidValue

NotConnected

AuthenticationFailed

PermissionDenied

Timeout

Busy

Disabled

ConfigurationClosed

NotConfigured

UnknownError
```

---

## FirebaseDataType

```cpp
Boolean

Integer

Float

Double

String

JSON

Array
```

---

## FirebaseEvent

```cpp
Connected

Disconnected

Authenticated

Ready

ReadCompleted

WriteCompleted

StreamUpdated

QueueFlushed

Error
```

---

# Typical Usage

```cpp
#include "Cloud/FirebaseManager/FirebaseManager.h"

FirebaseManager firebase;

void setup()
{
    firebase.configure(
        API_KEY,
        DATABASE_URL,
        USER_EMAIL,
        USER_PASSWORD
    );

    firebase.begin();

    firebase.onEvent(
        onFirebaseEvent
    );
}

void loop()
{
    firebase.loop();
}

void onFirebaseEvent(
    FirebaseEvent event,
    const char* path,
    void* context
)
{
    // Handle Firebase events
}
```

---

# Best Practices

- Configure Firebase before calling `begin()`.
- Always call `loop()`.
- Use constant database paths instead of string literals.
- Never call the Firebase client library directly.
- Let `FirebaseManager` manage authentication and reconnection.
- Use the event callback instead of polling whenever possible.
- Keep application logic outside `FirebaseManager`.
