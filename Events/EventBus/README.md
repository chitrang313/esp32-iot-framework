# EventBus

A reusable publish-subscribe communication manager for the ESP32 IoT Framework.

EventBus allows framework modules to communicate without directly depending on each other.

It manages event publishing, subscriptions, and asynchronous event dispatching.

---

# Features

* Publish events
* Subscribe to events
* Unsubscribe listeners
* Multiple subscribers
* Fixed-size event queue
* FIFO event processing
* Non-blocking dispatch
* Event validation
* Event statistics
* Framework-friendly architecture

---

# Create the manager

```cpp
EventBus eventBus;
```

---

# Initialize

```cpp
eventBus.begin();
```

---

# Subscribe

```cpp
eventBus.subscribe(
    EventType::RelayStateChanged,
    relayChangedCallback
);
```

---

# Unsubscribe

```cpp
eventBus.unsubscribe(
    EventType::RelayStateChanged,
    relayChangedCallback
);
```

---

# Publish Event

```cpp
RelayEvent relayEvent;

eventBus.publish(
    EventType::RelayStateChanged,
    EventSource::RelayManager,
    &relayEvent,
    sizeof(relayEvent)
);
```

---

# Main Loop

```cpp
void loop()
{
    eventBus.loop();
}
```

---

# Framework Lifecycle

```cpp
eventBus.begin();

eventBus.loop();

eventBus.enable();

eventBus.disable();

eventBus.isEnabled();
```

---

# Event Flow

```text
RelayManager
        │
        ▼
     EventBus
        │
 ┌──────┼──────────┐
 ▼      ▼          ▼
Logger  Sync     Application
```

---

# Notes

* Event publishing is non-blocking.
* Events are processed in FIFO order.
* Events are dispatched inside `loop()`.
* Multiple subscribers can receive the same event.
* EventBus does not control hardware.
* EventBus does not own runtime state.
* EventBus only delivers events between framework modules.
