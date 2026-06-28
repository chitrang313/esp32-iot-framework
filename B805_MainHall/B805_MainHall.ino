/**
 * ============================================================================
 *                       HOME AUTOMATION FIRMWARE
 * ============================================================================
 *   House     : B805  (adajan)
 *   Room      : Main Hall
 *   Board     : Board 1  •  4 individually-wired relays
 *   Device ID : PdMHOx6Pg6EIo9tmpMvV   (DO NOT CHANGE)
 * ----------------------------------------------------------------------------
 *   Contact Persons:
 *    - aarti
 *    - chitrang313
 *    - pooja
 * ----------------------------------------------------------------------------
 *   RELAY MAP  (matches your physical wiring)
 *   RELAY1 (GPIO 23 ) → main light 111 [Touch Switch]
 *   RELAY2 (GPIO 19 ) → Light [Touch Switch]
 *   RELAY3 (GPIO 18 ) → Inner-Entrance-Light [Touch Switch]
 *   RELAY4 (GPIO 5  ) → Entrance Light [Touch Switch]
 * ----------------------------------------------------------------------------
 *   Generated : 27 Jun 2026, 4:34 AM
 *   Flash ONLY to the ESP32 installed in "Main Hall" of "B805".
 *
 *   WARNING: This file contains Wi-Fi credentials and a Firebase service
 *   account. Do NOT share, commit, or upload it anywhere public.
 * ============================================================================
 */
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// ─── Wi-Fi credentials (entered at firmware download) ────────────────────
const char* WIFI_SSID     = "📡";
const char* WIFI_PASSWORD = "password";

// ─── Firebase project & dedicated device account ─────────────────────────
#define API_KEY        "REDACTED_GOOGLE_API_KEY"
#define DATABASE_URL   "REDACTED_FIREBASE_DB_URL"
#define USER_EMAIL     "REDACTED_EMAIL"
#define USER_PASSWORD  ""

// ─── Relay GPIO pins (active LOW: digitalWrite LOW = relay ON) ────────────
#define RELAY1_PIN  23    // main light 111
#define RELAY2_PIN  19    // Light
#define RELAY3_PIN  18    // Inner-Entrance-Light
#define RELAY4_PIN  5    // Entrance Light

// ─── Switch GPIO pins (only relays with a physical switch) ───────────────
#define SWITCH1_PIN  34    // main light 111 (Touch)
#define SWITCH2_PIN  35    // Light (Touch)
#define SWITCH3_PIN  33    // Inner-Entrance-Light (Touch)
#define SWITCH4_PIN  32    // Entrance Light (Touch)

// ─── RTDB paths (auto-generated, must match the dashboard) ───────────────
// The device STREAMS the parent node (instant push from the dashboard) and
// writes individual children when a physical switch toggles a relay.
#define RTDB_RELAYS_BASE  "/devices/PdMHOx6Pg6EIo9tmpMvV/relays"
#define RTDB_RELAY1  "/devices/PdMHOx6Pg6EIo9tmpMvV/relays/relay1"
#define RTDB_RELAY2  "/devices/PdMHOx6Pg6EIo9tmpMvV/relays/relay2"
#define RTDB_RELAY3  "/devices/PdMHOx6Pg6EIo9tmpMvV/relays/relay3"
#define RTDB_RELAY4  "/devices/PdMHOx6Pg6EIo9tmpMvV/relays/relay4"

// ─── Timing constants ────────────────────────────────────────────────────
const unsigned long TOUCH_DEBOUNCE_MS   = 150;    // capacitive TTP223
const unsigned long CLICK_DEBOUNCE_MS   = 50;     // mechanical button
const unsigned long WIFI_BOOT_TIMEOUT_MS    = 30000;  // give up + restart if Wi-Fi won't join at boot
const unsigned long WIFI_LOST_RESTART_MS    = 120000; // restart if Wi-Fi stays down this long at runtime
const unsigned long PENDING_PUSH_RETRY_MS   = 2000;   // retry interval for unsynced local toggles

// ─── Firebase client objects ─────────────────────────────────────────────
// fbdo   — request channel for writes (switch toggles, pending re-syncs)
// stream — dedicated channel for the RTDB stream (library requirement)
FirebaseData   fbdo;
FirebaseData   stream;
FirebaseAuth   auth;
FirebaseConfig config;

// ─── Channel tables (index i = one relay channel) ─────────────────────────
#define CH_COUNT 4
const uint8_t CH_PIN[CH_COUNT]   = { RELAY1_PIN, RELAY2_PIN, RELAY3_PIN, RELAY4_PIN };
const char*   CH_KEY[CH_COUNT]   = { "relay1", "relay2", "relay3", "relay4" };   // RTDB child key
const char*   CH_PATH[CH_COUNT]  = { RTDB_RELAY1, RTDB_RELAY2, RTDB_RELAY3, RTDB_RELAY4 };  // full RTDB path
const char*   CH_NAME[CH_COUNT]  = { "main light 111", "Light", "Inner-Entrance-Light", "Entrance Light" };

// Set when a local (wall-switch) toggle could not be pushed to RTDB —
// retried until it succeeds; stream updates for that channel are ignored
// meanwhile so a reconnect can't silently revert a real-world switch press.
bool pendingPush[CH_COUNT] = { false, false, false, false };

// ─── Per-channel ISR state (volatile — touched from interrupt context) ───
volatile bool          switchPending[CH_COUNT] = { false, false, false, false };
volatile unsigned long lastSwitchMs[CH_COUNT]  = { 0, 0, 0, 0 };

// main light 111 — Touch Switch (channel 0)
void IRAM_ATTR switch1ISR() {
  unsigned long now = millis();
  if (now - lastSwitchMs[0] > TOUCH_DEBOUNCE_MS) {
    switchPending[0] = true;
    lastSwitchMs[0]  = now;
  }
}

// Light — Touch Switch (channel 1)
void IRAM_ATTR switch2ISR() {
  unsigned long now = millis();
  if (now - lastSwitchMs[1] > TOUCH_DEBOUNCE_MS) {
    switchPending[1] = true;
    lastSwitchMs[1]  = now;
  }
}

// Inner-Entrance-Light — Touch Switch (channel 2)
void IRAM_ATTR switch3ISR() {
  unsigned long now = millis();
  if (now - lastSwitchMs[2] > TOUCH_DEBOUNCE_MS) {
    switchPending[2] = true;
    lastSwitchMs[2]  = now;
  }
}

// Entrance Light — Touch Switch (channel 3)
void IRAM_ATTR switch4ISR() {
  unsigned long now = millis();
  if (now - lastSwitchMs[3] > TOUCH_DEBOUNCE_MS) {
    switchPending[3] = true;
    lastSwitchMs[3]  = now;
  }
}


// ─── Relay helpers ───────────────────────────────────────────────────────
// Active-LOW wiring: digitalWrite LOW = relay ON.

void applyRelay(int ch, bool on) {
  digitalWrite(CH_PIN[ch], on ? LOW : HIGH);
}

bool relayIsOn(int ch) {
  return digitalRead(CH_PIN[ch]) == LOW;
}

// Toggle from a physical switch. The relay reacts INSTANTLY (works fully
// offline); the new state is then pushed to RTDB. If the push fails the
// channel is marked pendingPush and retried from loop() until it lands.
void toggleChannel(int ch) {
  bool newOn = !relayIsOn(ch);
  applyRelay(ch, newOn);
  if (Firebase.ready() && Firebase.RTDB.setBool(&fbdo, CH_PATH[ch], newOn)) {
    pendingPush[ch] = false;
  } else {
    pendingPush[ch] = true;
    Serial.printf("[sync] %s toggle queued (offline) — will push when online\n", CH_NAME[ch]);
  }
}

// ─── Firebase auth diagnostics ───────────────────────────────────────────
// Bounded retries + a clear serial message stop the endless sign-in storm
// that triggers Firebase's auth/too-many-requests lockout when credentials
// are wrong.
void onTokenStatus(TokenInfo info) {
  if (info.status == token_status_error) {
    Serial.printf("[auth] FAILED (code %d): %s\n",
                  info.error.code, info.error.message.c_str());
    Serial.println("[auth] Check USER_EMAIL / USER_PASSWORD, then re-flash.");
  } else if (info.status == token_status_ready) {
    Serial.println("[auth] Firebase sign-in OK.");
  }
}

// ─── RTDB stream callbacks ───────────────────────────────────────────────
// The stream delivers (a) one full JSON snapshot on every (re)connect and
// (b) tiny per-key updates the instant the dashboard toggles something.

void applyStreamValue(const char* key, bool on) {
  for (int ch = 0; ch < CH_COUNT; ch++) {
    if (strcmp(key, CH_KEY[ch]) != 0) continue;
    if (pendingPush[ch]) return;            // local change is newer — keep it
    if (relayIsOn(ch) != on) {
      applyRelay(ch, on);
      Serial.printf("[rtdb] %s -> %s\n", CH_NAME[ch], on ? "ON" : "OFF");
    }
    return;
  }
}

void streamCallback(FirebaseStream data) {
  if (data.dataTypeEnum() == firebase_rtdb_data_type_boolean) {
    // Delta: dataPath() is "/relayN"
    applyStreamValue(data.dataPath().c_str() + 1, data.boolData());
  } else if (data.dataTypeEnum() == firebase_rtdb_data_type_json) {
    // Initial snapshot (or multi-key update): sync every known channel.
    FirebaseJson* json = data.to<FirebaseJson*>();
    FirebaseJsonData item;
    for (int ch = 0; ch < CH_COUNT; ch++) {
      if (json->get(item, CH_KEY[ch]) && item.success) {
        applyStreamValue(CH_KEY[ch], item.boolValue);
      }
    }
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) Serial.println("[rtdb] stream timed out — resuming…");
  if (!stream.httpConnected()) {
    Serial.printf("[rtdb] stream error %d: %s\n",
                  stream.httpCode(), stream.errorReason().c_str());
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Home Automation firmware…");

  // ─── Relay outputs (start OFF; RTDB snapshot restores state on connect) ─
  for (int ch = 0; ch < CH_COUNT; ch++) {
    pinMode(CH_PIN[ch], OUTPUT);
    digitalWrite(CH_PIN[ch], HIGH);
  }

  // ─── Switch inputs + interrupts ───────────────────────────────────────
  pinMode(SWITCH1_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SWITCH1_PIN), switch1ISR, FALLING);
  pinMode(SWITCH2_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SWITCH2_PIN), switch2ISR, FALLING);
  pinMode(SWITCH3_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SWITCH3_PIN), switch3ISR, FALLING);
  pinMode(SWITCH4_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SWITCH4_PIN), switch4ISR, FALLING);

  // ─── Wi-Fi ────────────────────────────────────────────────────────────
  // setSleep(false): modem-sleep causes spurious interrupts on GPIO 36/39
  // (ESP32 errata) AND adds latency — always disable it on relay boards.
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiStart > WIFI_BOOT_TIMEOUT_MS) {
      Serial.println("\nWi-Fi failed — check SSID/password. Restarting in 5 s…");
      delay(5000);
      ESP.restart();
    }
    Serial.print('.');
    delay(250);
  }
  Serial.printf(" connected (%s).\n", WiFi.localIP().toString().c_str());

  // ─── Firebase ─────────────────────────────────────────────────────────
  config.api_key       = API_KEY;
  config.database_url  = DATABASE_URL;
  auth.user.email      = USER_EMAIL;
  auth.user.password   = USER_PASSWORD;
  // Bounded sign-in retries + status callback: with wrong credentials the
  // device logs a clear error and stops, instead of hammering Firebase Auth
  // until the account is rate-limited (auth/too-many-requests).
  config.max_token_generation_retry = 5;
  config.token_status_callback      = onTokenStatus;
  // Right-sized TLS buffers (default 16 KB rx eats RAM for no benefit here).
  fbdo.setBSSLBufferSize(2048, 1024);
  stream.setBSSLBufferSize(2048, 1024);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // ─── Live RTDB stream (instant push — no polling) ─────────────────────
  if (!Firebase.RTDB.beginStream(&stream, RTDB_RELAYS_BASE)) {
    Serial.printf("[rtdb] stream begin failed: %s\n", stream.errorReason().c_str());
  }
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  Serial.println("Setup complete — waiting for Firebase token…");
}

unsigned long lastPushRetry = 0;
unsigned long wifiLostSince  = 0;

void loop() {
  // 1. Physical switches — relays react instantly, even with no internet.
  if (switchPending[0]) { switchPending[0] = false; toggleChannel(0); }
  if (switchPending[1]) { switchPending[1] = false; toggleChannel(1); }
  if (switchPending[2]) { switchPending[2] = false; toggleChannel(2); }
  if (switchPending[3]) { switchPending[3] = false; toggleChannel(3); }

  // 2. Re-push local toggles that happened while offline (RTDB catch-up).
  if (Firebase.ready() && millis() - lastPushRetry >= PENDING_PUSH_RETRY_MS) {
    lastPushRetry = millis();
    for (int ch = 0; ch < CH_COUNT; ch++) {
      if (pendingPush[ch] && Firebase.RTDB.setBool(&fbdo, CH_PATH[ch], relayIsOn(ch))) {
        pendingPush[ch] = false;
        Serial.printf("[sync] %s pushed after reconnect\n", CH_NAME[ch]);
      }
    }
  }

  // 3. Wi-Fi watchdog — if the network stays gone too long, a clean restart
  //    recovers stuck TLS sessions. Physical switches keep working meanwhile.
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiLostSince == 0) wifiLostSince = millis();
    else if (millis() - wifiLostSince > WIFI_LOST_RESTART_MS) {
      Serial.println("Wi-Fi lost too long — restarting.");
      ESP.restart();
    }
  } else {
    wifiLostSince = 0;
  }
}
