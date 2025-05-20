#include <Preferences.h>
#include <WiFi.h>

// =====================[ KONFIGURATION ]======================

const bool IS_MASTER = true;

Preferences globals;
String DEVICE_ID = "UNDEFINED";

const char* SSID_MASTER     = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

// ==== RGB STATUS-LED (KY-016, gemeinsame Kathode) ====
const int STATUS_LED_R_PIN = 27;
const int STATUS_LED_G_PIN = 14;
const int STATUS_LED_B_PIN = 26;

// ==== Blinksteuerung ====
unsigned long last_blink = 0;
bool led_on = true;

// ==== Timer für loop() ====
unsigned long last_wifi_check = 0;
unsigned long last_master_report = 0;

// =====================[ FUNKTIONSPROTOTYPEN ]======================

bool connectToWiFi(bool isReconnect = false);
void setupAsMaster();
void setupAsSlave();
void checkAndReconnectWiFi();
String getDeviceId();
void updateStatusLED(bool isMaster);
void setColor(bool r, bool g, bool b);

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // LED vorbereiten
  pinMode(STATUS_LED_R_PIN, OUTPUT);
  pinMode(STATUS_LED_G_PIN, OUTPUT);
  pinMode(STATUS_LED_B_PIN, OUTPUT);

  // DEVICE_ID laden
  DEVICE_ID = getDeviceId();

  Serial.println("\n--- ESP32 Lampennetzwerk Start ---");
  Serial.println("device_id: " + DEVICE_ID);

  if (IS_MASTER) {
    setupAsMaster();
  } else {
    setupAsSlave();
  }
}

// =====================[ LOOP (tickbasiert) ]=============================

void loop() {
  unsigned long now = millis();

  // 10s Takt für WLAN-Reconnect bei Slaves
  if (!IS_MASTER && now - last_wifi_check > 10000) {
    checkAndReconnectWiFi();
    last_wifi_check = now;
  }

  // 10s Takt für Client-Status-Ausgabe beim Master
  if (IS_MASTER && now - last_master_report > 10000) {
    int clientCount = WiFi.softAPgetStationNum();
    Serial.printf("[MASTER] Verbundene Geräte: %d\n", clientCount);
    last_master_report = now;
  }

  // LED-Status immer aktuell
  updateStatusLED(IS_MASTER);

  // Platzhalter für spätere Logik:
  // lampLogic();
}

// =====================[ MASTER ]=============================

void setupAsMaster() {
  Serial.println("[MASTER] Starte Access Point...");
  WiFi.softAP(SSID_MASTER, PASSWORD_MASTER);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("[MASTER] Access Point IP: ");
  Serial.println(IP);
}

// =====================[ SLAVE ]=============================

void setupAsSlave() {
  Serial.println("[SLAVE] Initialisiere WLAN-Verbindung...");
  connectToWiFi(); // Initialer Versuch
}

bool connectToWiFi(bool isReconnect) {
  if (!isReconnect) {
    Serial.printf("[SLAVE #%s] Verbinde mit WLAN '%s'...\n", DEVICE_ID.c_str(), SSID_MASTER);
  }

  WiFi.begin(SSID_MASTER, PASSWORD_MASTER);

  const uint8_t maxRetries = isReconnect ? 5 : 20;
  const uint16_t retryDelay = 500;
  uint8_t retries = 0;

  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    delay(retryDelay);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[SLAVE] WLAN-Verbindung erfolgreich.");
    Serial.printf("[SLAVE] IP-Adresse: %s\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    Serial.println("\n[SLAVE] Verbindung fehlgeschlagen.");
    return false;
  }
}

void checkAndReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SLAVE] WLAN getrennt. Versuche Reconnect...");
    WiFi.disconnect(true);
    delay(1000);
    connectToWiFi(true);
  }
}

// =====================[ NVS: DEVICE_ID ]=============================

String getDeviceId() {
  String id = "UNDEFINED";
  globals.begin("globals", true);
  if (globals.isKey("device_id")) {
    id = globals.getString("device_id");
    if (id.length() == 0) id = "UNDEFINED";
  }
  globals.end();
  return id;
}

// =====================[ LED-Statusanzeige ]=============================

void updateStatusLED(bool isMaster) {
  bool r = false, g = false, b = false;
  bool blink = false;

  if (isMaster) {
    g = true;
    blink = (WiFi.softAPgetStationNum() == 0);
  } else {
    b = true;
    blink = (WiFi.status() != WL_CONNECTED);
  }

  if (blink) {
    if (millis() - last_blink > 1000) {
      led_on = !led_on;
      last_blink = millis();
    }
  } else {
    led_on = true;
  }

  setColor(led_on ? r : false, led_on ? g : false, led_on ? b : false);
}

void setColor(bool r, bool g, bool b) {
  digitalWrite(STATUS_LED_R_PIN, r ? HIGH : LOW);
  digitalWrite(STATUS_LED_G_PIN, g ? HIGH : LOW);
  digitalWrite(STATUS_LED_B_PIN, b ? HIGH : LOW);
}