#include <Preferences.h>
#include <WiFi.h>

// =====================[ KONFIGURATION ]======================

// Rolle: true = Master (Access Point), false = Slave (Client)
const bool IS_MASTER = true;

// NVS-Speicherobjekt & device_id als String
Preferences globals;
String DEVICE_ID = "UNDEFINED";

// WLAN-Zugangsdaten (für Master und Slave)
const char* SSID_MASTER     = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

// =====================[ Funktionsdeklarationen ]======================

bool connectToWiFi(bool isReconnect = false);
void setupAsMaster();
void setupAsSlave();
void checkAndReconnectWiFi();

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // DEVICE_ID aus NVS lesen
  globals.begin("globals", true);
  DEVICE_ID = globals.getString("device_id", "UNDEFINED");
  globals.end();

  Serial.println("\n--- ESP32 Lampennetzwerk Start ---");
  Serial.println("device_id: " + DEVICE_ID);

  if (IS_MASTER) {
    setupAsMaster();
  } else {
    setupAsSlave();
  }
}

// =====================[ MASTER-MODUS (Access Point) ]=============================

void setupAsMaster() {
  Serial.println("[MASTER] Starte Access Point...");

  WiFi.softAP(SSID_MASTER, PASSWORD_MASTER);
  IPAddress IP = WiFi.softAPIP();

  Serial.print("[MASTER] Access Point IP: ");
  Serial.println(IP);
}

// =====================[ SLAVE-MODUS (Client) ]=============================

void setupAsSlave() {
  Serial.println("[SLAVE] Initialisiere WLAN-Verbindung...");
  connectToWiFi(); // Erste Verbindung
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

// =====================[ LOOP ]=============================

void loop() {
  if (IS_MASTER) {
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 3000) {
      int clientCount = WiFi.softAPgetStationNum();
      Serial.printf("[MASTER] Verbundene Geräte: %d\n", clientCount);
      lastReport = millis();
    }
  } else {
    checkAndReconnectWiFi();
  }

  delay(10); // CPU-Entlastung
}