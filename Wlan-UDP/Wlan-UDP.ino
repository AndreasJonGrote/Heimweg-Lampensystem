#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// =====================[ DEVICE SETTINGS ]======================

bool IS_MASTER = true;
bool IS_STANDALONE = false;
bool IS_SLAVE = false ;

Preferences globals;
String DEVICE_ID = "UNDEFINED";

// =====================[ WLAN SETTINGS ]======================

const char* SSID_MASTER     = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

IPAddress master_ip(192, 168, 4, 1);

unsigned long last_wlan_task = 0;

// =====================[ PUSH BUTTON ]======================

const int BUTTON_PUSH_PIN = 19; // Push Button – Reboot

unsigned long button_pressed_since = 0;

// =====================[ RGB STATUS-LED ]======================

const int STATUS_LED_R_PIN = 27;
const int STATUS_LED_G_PIN = 14;
const int STATUS_LED_B_PIN = 26;

// ==== Status-LED Logik ====
const char* status_led_color = "red";  // "red", "green", "blue", "white", "off"
bool status_led_blink = false;
unsigned long last_blink = 0;
bool led_on = true;

// ==== Fast-Blink-Logik ====
bool fast_blink_active = false;
unsigned long fast_blink_last_toggle = 0;
int fast_blink_count = 0;

void triggerFastBlink() {
  fast_blink_active = true;
  fast_blink_last_toggle = millis();
  fast_blink_count = 0;
  led_on = false;  // optional für sauberen Start
}


// =====================[ UDP ]======================

WiFiUDP udp;
const uint16_t UDP_PORT = 4210;

// ==== Maximale Anzahl Slaves ====
const int MAX_SLAVES = 25;

// ==== Slave-Datenstruktur ====
struct SlaveInfo {
  String id;
  IPAddress ip;
};

// ==== Array zur Speicherung ====
SlaveInfo slaves[MAX_SLAVES];
int slave_count = 0;

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);

  delay(500);

  // PIN MODES
  pinMode(BUTTON_PUSH_PIN, INPUT_PULLUP);

  pinMode(STATUS_LED_R_PIN, OUTPUT);
  pinMode(STATUS_LED_G_PIN, OUTPUT);
  pinMode(STATUS_LED_B_PIN, OUTPUT);

  // BOOT COLOR
  setColor(true, false, true);

  DEVICE_ID = getDeviceId();

  Serial.print(">>> Boot Device: ");
  Serial.println(DEVICE_ID);

  bool existing_ap = checkExistingAP() ;

  Serial.print(">>> Role: ");

  if (IS_STANDALONE || (IS_MASTER && existing_ap)) {
    status_led_color = "white";
    if (IS_MASTER && existing_ap) {
      IS_MASTER = false ;
      IS_STANDALONE = true ;
      status_led_blink = true ;
    }
    Serial.println("STANDALONE");
  } else {
    if (IS_MASTER) {
      status_led_color = "green";
      Serial.println("MASTER");
      setupAsMaster();
    } else {
      status_led_color = "blue";
      IS_SLAVE = true ;
      Serial.println("SLAVE");
      setupAsSlave();
    }
  }

}

// =====================[ LOOP – Zeitbasiert ]=============================

void loop() {

  unsigned long now = millis();

  // === 1. WLAN-Handling ===
  if (!IS_STANDALONE) {

    if (IS_MASTER) {

      checkUdpMessages();

      if (now - last_wlan_task >= 15000 || last_wlan_task == 0) {
        last_wlan_task = now;
        int clientCount = WiFi.softAPgetStationNum();
        Serial.print("Verbundene Geräte: ");
        Serial.println(clientCount);

        // === Ausgabe der bekannten Slaves ===
        Serial.println(">>> Registrierte Slaves:");
        for (int i = 0; i < slave_count; i++) {
          Serial.print(" ID: ");
          Serial.print(slaves[i].id);
          Serial.print(" | IP: ");
          Serial.println(slaves[i].ip);
        }
        if (slave_count == 0) {
          Serial.println(" Keine Slaves registriert.");
        }

        status_led_color = "green";
        status_led_blink = (clientCount == 0);
      }
    }

    if (IS_SLAVE) {
      if (now - last_wlan_task >= 15000 || last_wlan_task == 0) {
        last_wlan_task = now;
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (!connected) {
          WiFi.disconnect(true);
          delay(100);
          connectToWiFi(true);
        } else {
          sendUdpPing();
        }

        status_led_color = "blue";
        status_led_blink = (WiFi.status() != WL_CONNECTED);
      }
    }

  }
  // === 2. Check Status ===
  checkStatus();

  // === 3. Lampenlogik (Platzhalter) ===
  // lampLogic();
}

// =====================[ LED Funktionsblock ]=============================

void statusLED() {
  bool r = false, g = false, b = false;

  // Farbe aus globaler status_led_color wählen
  if (strcmp(status_led_color, "red") == 0)    r = true;
  if (strcmp(status_led_color, "green") == 0)  g = true;
  if (strcmp(status_led_color, "blue") == 0)   b = true;
  if (strcmp(status_led_color, "white") == 0)  { r = true; g = true; b = true; }

  bool visible = true;

  // === 1. Schnelles Blinken überschreibt ALLES ===
  if (fast_blink_active) {
    if (millis() - fast_blink_last_toggle >= 50) {
      fast_blink_last_toggle = millis();
      fast_blink_count++;
      led_on = !led_on;
    }

    if (fast_blink_count >= 6) {
      fast_blink_active = false;
    }

    visible = led_on;
  }

  // === 2. Normales Blinken (z. B. bei Status-Blink) ===
  else if (status_led_blink) {
    if (millis() - last_blink >= 1000) {
      last_blink = millis();
      led_on = !led_on;
    }
    visible = led_on;
  }

  // === 3. LED setzen ===
  setColor(visible ? r : false, visible ? g : false, visible ? b : false);
}

void setColor(bool r, bool g, bool b) {
  digitalWrite(STATUS_LED_R_PIN, r ? HIGH : LOW);
  digitalWrite(STATUS_LED_G_PIN, g ? HIGH : LOW);
  digitalWrite(STATUS_LED_B_PIN, b ? HIGH : LOW);
}

// =====================[ WLAN & DEVICE_ID ]=============================

bool checkExistingAP() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // wichtig für frischen Scan
  delay(100);

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == SSID_MASTER) {
      WiFi.scanDelete(); // Aufräumen
      return true;
    }
  }

  WiFi.scanDelete(); // Aufräumen, falls nicht gefunden
  return false;
}

void setupAsMaster() {
  Serial.println("Starte Access Point...");
  WiFi.softAP(SSID_MASTER, PASSWORD_MASTER);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP: ");
  Serial.println(IP);

  udp.begin(UDP_PORT);
  Serial.print("[MASTER] UDP Listening on Port ");
  Serial.println(UDP_PORT);
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

// =====================[ UDP ]=============================
void sendUdpPing() {

  triggerFastBlink();

  udp.beginPacket(master_ip, UDP_PORT);
  udp.print("ping:" + DEVICE_ID);
  udp.endPacket();
}

void checkUdpMessages() {
  int packetSize = udp.parsePacket();
  if (packetSize) {

    triggerFastBlink();

    char incoming[64];
    int len = udp.read(incoming, sizeof(incoming) - 1);
    if (len > 0) incoming[len] = '\0';

    String message = String(incoming);
    IPAddress senderIP = udp.remoteIP();

    Serial.print("[MASTER] UDP empfangen: ");
    Serial.println(message);

    // Nur reagieren auf "ping:<DEVICE_ID>"
    if (message.startsWith("ping:")) {
      String id = message.substring(5);  // alles nach "ping:"

      // Prüfen, ob ID schon vorhanden ist
      bool exists = false;
      for (int i = 0; i < slave_count; i++) {
        if (slaves[i].id == id) {
          slaves[i].ip = senderIP;  // IP aktualisieren
          exists = true;
          break;
        }
      }

      // Neue ID speichern, falls noch nicht vorhanden
      if (!exists && slave_count < MAX_SLAVES) {
        slaves[slave_count].id = id;
        slaves[slave_count].ip = senderIP;
        slave_count++;

        Serial.print("[MASTER] Neuer Slave registriert: ");
        Serial.print(id);
        Serial.print(" > ");
        Serial.println(senderIP);
      }
    }
  }
}

// =====================[ BUTTON AND STATUS LED ]=============================

void checkStatus() {
  
  static bool was_pressed = false;
  static bool override_led = false;

  if (digitalRead(BUTTON_PUSH_PIN) == LOW) {
    if (!was_pressed) {
      button_pressed_since = millis();  // Startzeit merken
      was_pressed = true;
      override_led = true;
      setColor(true, false, false);  // ROT anzeigen
    }

    if (millis() - button_pressed_since >= 5000) {
      Serial.println(">>> BUTTON: Reset nach 5 Sekunden!");
      delay(100);
      ESP.restart();
    }

  } else {
    if (was_pressed) {
      unsigned long press_duration = millis() - button_pressed_since;

      if (press_duration < 1000) {
        Serial.println(">>> BUTTON: Kurz gedrückt (unter 1 Sekunde)");
        triggerFastBlink();
      }

      was_pressed = false;
      button_pressed_since = 0;
      override_led = false;
    }
  }

  // === REGULAR STATUS LED ===
  if (!override_led) {
    statusLED(); 
  }
}


// =====================[ CONFIG VARS ]=============================

String getDeviceId() {
  globals.begin("globals", true);
  String id = globals.getString("device_id", "UNDEFINED");
  globals.end();
  return id;
}