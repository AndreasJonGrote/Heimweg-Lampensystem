#include <WiFi.h>

// =====================[ KONFIGURATION ]======================

// Rolle: true = Master (Access Point), false = Slave (Client)
const bool IS_MASTER = false;

// Geräte-ID (z. B. für spätere Zuordnung)
const int DEVICE_ID = 1;

// WLAN-Daten (wenn Master: eigener AP, wenn Slave: damit verbinden)
const char* SSID_MASTER = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

// =====================[ SETUP ]=============================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- ESP32 Lampennetzwerk Start ---");

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

  delay(500);
}

// =====================[ SLAVE-MODUS (Client) ]=============================

bool connectToWiFi(bool isReconnect = false) {
  if (!isReconnect) {
    Serial.printf("[SLAVE #%d] Verbinde mit WLAN '%s'...\n", DEVICE_ID, SSID_MASTER);
  }

  WiFi.begin(SSID_MASTER, PASSWORD_MASTER);
  
  const uint8_t maxRetries = isReconnect ? 10 : 20;
  const uint8_t retryDelay = 500; // ms
  uint8_t retries = 0;
  
  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    delay(retryDelay);
    Serial.print(".");
    retries++;
  }

  const bool isConnected = (WiFi.status() == WL_CONNECTED);
  
  if (isConnected) {
    Serial.println(isReconnect ? "\n[SLAVE] Wiederverbindung erfolgreich!" : "\n[SLAVE] Verbunden!");
    Serial.printf("[SLAVE] IP-Adresse: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println(isReconnect ? "\n[SLAVE] Wiederverbindung fehlgeschlagen." : "\n[SLAVE] Verbindung fehlgeschlagen.");
  }
  
  return isConnected;
}

void setupAsSlave() {
  if (!connectToWiFi()) {
    Serial.println("[SLAVE] Initiale Verbindung fehlgeschlagen. Starte Neustart...");
    ESP.restart();
  }
}

void checkAndReconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SLAVE] WLAN-Verbindung verloren. Versuche Wiederverbindung...");
    WiFi.disconnect();
    delay(1000);
    
    if (!connectToWiFi(true)) {
      Serial.println("[SLAVE] Wiederverbindung fehlgeschlagen. Starte Neustart...");
      ESP.restart();
    }
  }
}

// =====================[ LOOP ]=============================

void loop() {

  // IF IS MASTER
  if (IS_MASTER) {
    // Anzahl der verbundenen Clients anzeigen
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 3000) {
      int clientCount = WiFi.softAPgetStationNum();
      Serial.printf("[MASTER] Verbundene Geräte: %d\n", clientCount);
      lastReport = millis();
    }
  }

  // IF IS SLAVE
  if (!IS_MASTER) {
    checkAndReconnectWiFi();
    delay(5000); // Prüfe alle 5 Sekunden die Verbindung
  }

  delay(10);
}