#include <Preferences.h>
#include <WiFi.h>

// =====================[ KONFIGURATION ]======================

const bool IS_MASTER = true;
const bool IS_STANDALONE = false;

Preferences globals;
String DEVICE_ID = "UNDEFINED";

const char* SSID_MASTER     = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

// ==== RGB STATUS-LED (KY-016, gemeinsame Kathode) ====
const int STATUS_LED_R_PIN = 27;
const int STATUS_LED_G_PIN = 14;
const int STATUS_LED_B_PIN = 26;

// ==== Status-LED Logik ====
const char* status_led_color = "off";  // "red", "green", "blue", "white", "off"
bool status_led_blink = false;
unsigned long last_blink = 0;
bool led_on = true;

// ==== Zeitsteuerung ====
unsigned long last_wlan_task = 0;

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(STATUS_LED_R_PIN, OUTPUT);
  pinMode(STATUS_LED_G_PIN, OUTPUT);
  pinMode(STATUS_LED_B_PIN, OUTPUT);

  DEVICE_ID = getDeviceId();

  Serial.print(">>> Boot Device: ");
  Serial.println(DEVICE_ID);

  Serial.print(">>> IS_MASTER: ");
  Serial.println(IS_MASTER);

  Serial.print(">>> IS_STANDALONE: ");
  Serial.println(IS_STANDALONE);



  if (IS_STANDALONE) {
    status_led_color = "white";
    if (IS_MASTER) {
      status_led_blink = true;
    }
  } else {
    if (IS_MASTER) {
      setupAsMaster();
      status_led_color = "green";
      status_led_blink = true;
    } else {
      setupAsSlave();
      status_led_color = "blue";
      status_led_blink = true;
    }
  }

}

// =====================[ LOOP – Zeitbasiert ]=============================

void loop() {
  unsigned long now = millis();

  // === 1. WLAN-Handling (alle 10s) ===
  if (now - last_wlan_task >= 10000) {
    last_wlan_task = now;
    if (!IS_STANDALONE) {
      if (IS_MASTER) {
        int clientCount = WiFi.softAPgetStationNum();
        Serial.print("Verbundene Geräte: ");
        Serial.println(clientCount);

        status_led_color = "green";
        status_led_blink = (clientCount == 0);
      } else {
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (!connected) {
          WiFi.disconnect(true);
          delay(100);
          connectToWiFi(true);
        }

        status_led_color = "blue";
        status_led_blink = (WiFi.status() != WL_CONNECTED);
      }
    }
  }

  // === 2. Status-LED in Echtzeit ===
  statusLED();

  // === 3. Lampenlogik (Platzhalter) ===
  // lampLogic();
}

// =====================[ LED Funktionsblock ]=============================

void statusLED() {
  bool r = false, g = false, b = false;

  if (strcmp(status_led_color, "red") == 0)    r = true;
  if (strcmp(status_led_color, "green") == 0)  g = true;
  if (strcmp(status_led_color, "blue") == 0)   b = true;
  if (strcmp(status_led_color, "white") == 0)  { r = true; g = true; b = true; }

  bool visible = true;
  if (status_led_blink) {
    if (millis() - last_blink > 1000) {
      led_on = !led_on;
      last_blink = millis();
    }
    visible = led_on;
  }

  setColor(visible ? r : false, visible ? g : false, visible ? b : false);
}

void setColor(bool r, bool g, bool b) {
  digitalWrite(STATUS_LED_R_PIN, r ? HIGH : LOW);
  digitalWrite(STATUS_LED_G_PIN, g ? HIGH : LOW);
  digitalWrite(STATUS_LED_B_PIN, b ? HIGH : LOW);
}

// =====================[ WLAN & DEVICE_ID ]=============================

void setupAsMaster() {
  Serial.println("Starte Access Point...");
  WiFi.softAP(SSID_MASTER, PASSWORD_MASTER);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP: ");
  Serial.println(IP);
}

void setupAsSlave() {
  Serial.println("[SLAVE] WLAN verbinden...");
  connectToWiFi(false);
}

bool connectToWiFi(bool isReconnect) {
  if (!isReconnect) {
    Serial.print("[SLAVE #");
    Serial.print(DEVICE_ID);
    Serial.print("] Verbinde mit ");
    Serial.println(SSID_MASTER);
  }

  WiFi.begin(SSID_MASTER, PASSWORD_MASTER);

  const int maxRetries = isReconnect ? 5 : 20;
  for (int i = 0; i < maxRetries; i++) {
    delay(500);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("[SLAVE] Verbunden mit IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.print(".");
  }

  Serial.println("\n[SLAVE] Verbindung fehlgeschlagen.");
  return false;
}

String getDeviceId() {
  globals.begin("globals", true);
  String id = globals.getString("device_id", "UNDEFINED");
  globals.end();
  return id;
}