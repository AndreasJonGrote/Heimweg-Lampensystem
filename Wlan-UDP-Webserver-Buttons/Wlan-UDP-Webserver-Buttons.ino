#include <Preferences.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>

// =====================[ INIT ]======================

Preferences globals;
String DEVICE_ID = "UNDEFINED";

// =====================[ WLAN SETTINGS ]======================

const char* SSID_MASTER     = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

IPAddress master_ip(192, 168, 4, 1);

unsigned long last_wlan_task = 0;

// =====================[ UDP ]======================

WiFiUDP udp;
const uint16_t UDP_PORT = 4210;

// ==== Maximale Anzahl Slaves ====
const int MAX_SLAVES = 25;

// ==== Slave-Datenstruktur ====
struct SlaveInfo {
  String id;                // DEVICE ID
  IPAddress ip;             // letzte bekannte IP
  unsigned long lastSeen;   // Zeitpunkt der letzten Nachricht (millis)

  bool dynamicMode;         // true = auf Sensorik reagieren
  uint8_t staticBrightness; // 0–90 (%), nur aktiv bei !dynamicMode
  uint8_t staticColorR;     // RGB-Farbe für statisches Licht
  uint8_t staticColorG;
  uint8_t staticColorB;
  bool dynamicColor;        // true = Farbwechsel aktiv (nur bei dynamicMode)
  String sensorMode;        // "ultrasound", "mic", "both"
};

// ==== Array zur Speicherung ====
SlaveInfo slaves[MAX_SLAVES];
int slave_count = 0;

// =====================[ WEBSERVER ]======================

WebServer server(80);

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="30">
  <title>ESP32 Übersicht</title>
  <style>
    body { font-family: sans-serif; padding: 20px; }
    table { border-collapse: collapse; width: 100%; }
    th, td { border: 1px solid #aaa; padding: 8px; text-align: center; }
    button { padding: 5px 10px; }
  </style>
  <script>
    function sendCmd(ip, cmd) {
      fetch(`/cmd?ip=${ip}&cmd=${cmd}`)
        .then(res => res.text())
        .then(msg => alert("Antwort: " + msg))
        .catch(err => alert("Fehler: " + err));
    }
  </script>
</head>
<body>
  <h1>ESP32 Geräte im Netzwerk</h1>
  <table>
    <thead>
      <tr><th>Device ID</th><th>IP</th><th>Befehl 1</th><th>Befehl 2</th><th>Befehl 3</th></tr>
    </thead>
    <tbody>
      %SLAVE_ROWS%
    </tbody>
  </table>
</body>
</html>
)rawliteral";

String generateSlaveRows() {
  String rows = "";

  // === Master (eigene Zeile) ===
  String masterIP = WiFi.softAPIP().toString();
  rows += "<tr><td>" + DEVICE_ID + " (Master)</td><td>" + masterIP + "</td>";
  rows += "<td><button onclick=\"sendCmd('" + masterIP + "', 'reboot')\">Reboot</button></td>";
  rows += "<td><button onclick=\"sendCmd('" + masterIP + "', 'setcolor:0,255,0')\">Grün</button></td>";
  rows += "<td><button onclick=\"sendCmd('" + masterIP + "', 'setcolor:0,0,255')\">Blau</button></td></tr>";

  // === Slaves ===
  for (int i = 0; i < slave_count; i++) {
    String ip = slaves[i].ip.toString();
    rows += "<tr><td>" + slaves[i].id + "</td><td>" + ip + "</td>";
    rows += "<td><button onclick=\"sendCmd('" + ip + "', 'reboot')\">Reboot</button></td>";
    rows += "<td><button onclick=\"sendCmd('" + ip + "', 'cmd2')\">CMD2</button></td>";
    rows += "<td><button onclick=\"sendCmd('" + ip + "', 'cmd3')\">CMD3</button></td></tr>";
  }

  return rows;
}

void handleRoot() {
  String page = htmlPage;
  page.replace("%SLAVE_ROWS%", generateSlaveRows());
  server.send(200, "text/html", page);
}

// =====================[ DEVICE SETTINGS ]======================

const int DIP_STANDALONE_PIN    = 13; // DIP 1 – WLAN aus (lokal)
const int DIP_ACCESS_POINT_PIN  = 25; // DIP 2 – Master: Access Point starten
const int DIP_DYNAMIC_MODE_PIN  = 33; // DIP 3 – Modus: dynamisch vs. statisch
const int DIP_DYNAMIC_COLOR_PIN = 32; // DIP 4 – Farbe: dynamisch vs. statisch

struct DipConfig {
  bool isStandalone;
  bool isMasterAP;
  bool isDynamicMode;
  bool isDynamicColor;
  bool hasSonicSensor;
  bool hasMicrophone;
};

DipConfig readDipConfig();
DipConfig config;

bool IS_STANDALONE    = false;
bool IS_MASTER        = false;
bool IS_SLAVE         = false;
bool IS_DYNAMIC_MODE  = false;
bool IS_DYNAMIC_COLOR = false;
bool HAS_SONIC_SENSOR = false;
bool HAS_MICROPHONE   = false;

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

// =====================[ VIBRATION ]======================

const int VIBRATION_MOTOR_PIN = 23;

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);

  delay(500);

  // PIN MODES
  pinMode(BUTTON_PUSH_PIN, INPUT_PULLUP);

  pinMode(DIP_STANDALONE_PIN,    INPUT_PULLUP);
  pinMode(DIP_ACCESS_POINT_PIN,  INPUT_PULLUP);
  pinMode(DIP_DYNAMIC_MODE_PIN,  INPUT_PULLUP);
  pinMode(DIP_DYNAMIC_COLOR_PIN, INPUT_PULLUP);

  pinMode(STATUS_LED_R_PIN, OUTPUT);
  pinMode(STATUS_LED_G_PIN, OUTPUT);
  pinMode(STATUS_LED_B_PIN, OUTPUT);

  pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
  digitalWrite(VIBRATION_MOTOR_PIN, LOW);
  void vibratePulse(int dauer_ms = 100);

  // BOOT COLOR
  setColor(true, false, true);

  // Device ID
  DEVICE_ID = getDeviceId();

  Serial.print(">>> Boot Device: ");
  Serial.println(DEVICE_ID);

  // Lade Schalter Konfig
  config = readDipConfig();

  // Globale Flags setzen
  IS_STANDALONE    = config.isStandalone;
  IS_MASTER        = config.isMasterAP;
  IS_DYNAMIC_MODE  = config.isDynamicMode;
  IS_DYNAMIC_COLOR = config.isDynamicColor;
  HAS_SONIC_SENSOR = config.hasSonicSensor;
  HAS_MICROPHONE   = config.hasMicrophone;
  
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
    
    server.on("/", handleRoot);

    server.on("/cmd", []() {
      if (server.hasArg("ip") && server.hasArg("cmd")) {
        String ipStr = server.arg("ip");
        String cmd = server.arg("cmd");
        IPAddress targetIP;
        
        if (targetIP.fromString(ipStr)) {
          if (targetIP == WiFi.softAPIP()) {
            // Lokale Ausführung direkt ohne UDP
            Serial.println(">>> Lokaler Befehl: " + cmd);
            if (cmd == "reboot") {
              delay(100);
              ESP.restart();
            }
            // Weitere Kommandos hier abfangen
          } else {
            sendUdpCommand(targetIP, cmd);
          }

          server.send(200, "text/plain", "Befehl gesendet.");
        } else {
          server.send(400, "text/plain", "Ungültige IP-Adresse.");
        }
      } else {
        server.send(400, "text/plain", "Fehlende Parameter.");
      }
    });

    server.begin();
    Serial.println("HTTP Server gestartet");
  
  }

  // === IMPULSE
  vibratePulse();

}

// =====================[ LOOP – Zeitbasiert ]=============================

void loop() {

  unsigned long now = millis();

  // === 1. WLAN-Handling ===
  if (!IS_STANDALONE) {

    server.handleClient();
    checkUdpMessages();

    if (IS_MASTER) {

      if (now - last_wlan_task >= 15000 || last_wlan_task == 0) {
        last_wlan_task = now;
        int clientCount = WiFi.softAPgetStationNum();
        Serial.print("Verbundene Geräte: ");
        Serial.println(clientCount);

        // === Alte Slaves entfernen ===
        unsigned long now = millis();
        for (int i = 0; i < slave_count; ) {
          if (now - slaves[i].lastSeen > 29000) {
            Serial.print("[INFO] Slave entfernt (Timeout): ");
            Serial.println(slaves[i].id);

            // Nachrücklogik: letzten Eintrag nach vorne holen
            for (int j = i; j < slave_count - 1; j++) {
              slaves[j] = slaves[j + 1];
            }
            slave_count--;
          } else {
            i++;  // Nur erhöhen, wenn nicht gelöscht wurde
          }
        }

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
        status_led_blink = (slave_count == 0);
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
          sendUdpStatus();
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

  udp.begin(UDP_PORT);
  Serial.print("[MASTER] UDP Listening on Port ");
  Serial.println(UDP_PORT);
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
void sendUdpStatus() {
  triggerFastBlink();

  // Beispielwerte (später dynamisch befüllen!)
  uint8_t staticColorR = 0;
  uint8_t staticColorG = 0;
  uint8_t staticColorB = 0;
  String sensorMode = "ultra";  // "mic", "ultra", "both"

  String payload = "status:";
  payload += DEVICE_ID;
  payload += ",";
  payload += IS_DYNAMIC_MODE ? "1" : "0";
  payload += ",";
  payload += IS_DYNAMIC_COLOR ? "1" : "0";
  payload += ",";
  payload += String(staticColorR) + "," + String(staticColorG) + "," + String(staticColorB);
  payload += ",";
  payload += sensorMode;

  udp.beginPacket(master_ip, UDP_PORT);
  udp.print(payload);
  udp.endPacket();
}

void sendUdpCommand(IPAddress ip, String command) {
  udp.beginPacket(ip, UDP_PORT);
  udp.print("command:" + command);
  udp.endPacket();

  Serial.print(">>> UDP-Befehl an ");
  Serial.print(ip);
  Serial.print(": ");
  Serial.println(command);
}

void checkUdpMessages() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {

    triggerFastBlink();

    char incoming[64];
    int len = udp.read(incoming, sizeof(incoming) - 1);
    if (len > 0) {
      incoming[len] = '\0'; // Nullterminierung
    }

    String message = String(incoming);
    IPAddress senderIP = udp.remoteIP();

    Serial.print("[UDP] Nachricht empfangen von ");
    Serial.print(senderIP);
    Serial.print(": ");
    Serial.println(message);

    // === STATUS-Nachricht ===
    if (message.startsWith("status:")) {
      String payload = message.substring(7);  // Alles nach "status:"

      // Zerlegen der Daten
      int idx = 0;
      String parts[8];
      while (payload.length() > 0 && idx < 8) {
        int sep = payload.indexOf(',');
        if (sep == -1) {
          parts[idx++] = payload;
          break;
        } else {
          parts[idx++] = payload.substring(0, sep);
          payload = payload.substring(sep + 1);
        }
      }

      if (idx >= 7) {
        String id = parts[0];
        bool exists = false;

        for (int i = 0; i < slave_count; i++) {
          if (slaves[i].id == id) {
            slaves[i].ip = senderIP;
            slaves[i].lastSeen = millis();
            slaves[i].dynamicMode = parts[1] == "1";
            slaves[i].dynamicColor = parts[2] == "1";
            slaves[i].staticColorR = parts[3].toInt();
            slaves[i].staticColorG = parts[4].toInt();
            slaves[i].staticColorB = parts[5].toInt();
            slaves[i].sensorMode = parts[6];
            exists = true;
            break;
          }
        }

        if (!exists && slave_count < MAX_SLAVES) {
          int i = slave_count++;
          slaves[i].id = parts[0];
          slaves[i].ip = senderIP;
          slaves[i].lastSeen = millis();
          slaves[i].dynamicMode = parts[1] == "1";
          slaves[i].dynamicColor = parts[2] == "1";
          slaves[i].staticColorR = parts[3].toInt();
          slaves[i].staticColorG = parts[4].toInt();
          slaves[i].staticColorB = parts[5].toInt();
          slaves[i].sensorMode = parts[6];

          Serial.print("[UDP] Neuer Slave registriert: ");
          Serial.print(parts[0]);
          Serial.print(" @ ");
          Serial.println(senderIP);
        }
      } else {
        Serial.println("[UDP] Ungültige STATUS-Nachricht empfangen");
      }
    }

    // === COMMAND-Verarbeitung ===
    else if (message.startsWith("command:")) {
      String cmd = message.substring(8);

      Serial.print("[UDP] Befehl erhalten: ");
      Serial.println(cmd);

      // Zentrale Befehlsverarbeitung
      if (cmd == "reboot") {
        Serial.println(">>> Reboot wird ausgeführt...");
        delay(100);
        ESP.restart();
      }

      // Weitere Befehle hier ergänzen:
      // else if (cmd == "setcolor:0,255,0") { ... }

      else {
        Serial.println(">>> Unbekannter Befehl");
      }
    }

    // === Unbekanntes Format ===
    else {
      Serial.println("[UDP] Unbekannte Nachricht empfangen.");
    }
  }
}

// =====================[ VIBRATION ]=============================

void vibratePulse(int dauer_ms = 250) {
  digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
  delay(dauer_ms);
  digitalWrite(VIBRATION_MOTOR_PIN, LOW);
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
      vibratePulse(); // haptisches feedback
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

DipConfig readDipConfig() {
  DipConfig cfg;
  cfg.isStandalone   = digitalRead(DIP_STANDALONE_PIN)    == LOW;
  cfg.isMasterAP     = digitalRead(DIP_ACCESS_POINT_PIN)  == LOW;
  cfg.isDynamicMode  = digitalRead(DIP_DYNAMIC_MODE_PIN)  == LOW;
  cfg.isDynamicColor = digitalRead(DIP_DYNAMIC_COLOR_PIN) == LOW;
  cfg.hasSonicSensor = digitalRead(DIP_ENABLE_SONIC_PIN) == LOW;
  cfg.hasMicrophone  = digitalRead(DIP_ENABLE_MIC_PIN) == LOW;
  return cfg;
}

String getDeviceId() {
  globals.begin("globals", true);
  String id = globals.getString("device_id", "UNDEFINED");
  globals.end();
  return id;
}