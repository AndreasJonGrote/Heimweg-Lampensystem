/*
   ESP32 Lampennetzwerk – Master/Slave Setup mit Webinterface
   Stand: Mai 2025
   Von: Jon & ChatGPT
*/

#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <esp_netif.h>

// =====================[ KONFIGURATION ]======================

// Rolle: true = Master (Access Point), false = Slave (Client)
const bool IS_MASTER = true;

// Geräte-ID (z.B. für spätere Zuordnung)
const uint8_t DEVICE_ID = 3;

// WLAN-Daten (wenn Master: eigener AP, wenn Slave: damit verbinden)
const char* SSID_MASTER = "LAMPEN_NETZWERK";
const char* PASSWORD_MASTER = "geheim123";

// Webserver auf Port 80 (nur Master!)
WebServer server(80);

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

  // Webserver-Routen definieren
  server.on("/", handleRoot);
  server.begin();
  Serial.println("[MASTER] Webserver gestartet.");
}

// =====================[ SLAVE-MODUS (Client) ]=============================

void setupAsSlave() {
  Serial.printf("[SLAVE #%d] Verbinde mit WLAN '%s'...\n", DEVICE_ID, SSID_MASTER);

  WiFi.begin(SSID_MASTER, PASSWORD_MASTER);

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[SLAVE] Verbunden!");
    Serial.print("[SLAVE] IP-Adresse: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[SLAVE] Verbindung fehlgeschlagen.");
  }
}

// =====================[ WEBSEITE: Verbundene Clients anzeigen ]=============================

void handleRoot() {
  wifi_sta_list_t stationList;
  wifi_sta_info_t stations[10];  // Maximale Anzahl von Clients
  stationList.num = 0;
  stationList.sta = stations;

  String html = "<!DOCTYPE html><html><head>"
                "<meta charset='UTF-8'>"
                "<title>ESP32 Lampennetzwerk</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; margin: 20px; }"
                "h2 { color: #333; }"
                "ul { list-style-type: none; padding: 0; }"
                "li { margin: 10px 0; padding: 10px; background: #f0f0f0; border-radius: 5px; }"
                "</style></head><body>"
                "<h2>Verbundene Geräte:</h2><ul>";

  if (esp_wifi_ap_get_sta_list(&stationList) == ESP_OK) {
    for (int i = 0; i < stationList.num; i++) {
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               stationList.sta[i].mac[0], stationList.sta[i].mac[1],
               stationList.sta[i].mac[2], stationList.sta[i].mac[3],
               stationList.sta[i].mac[4], stationList.sta[i].mac[5]);
      
      html += "<li>Client #" + String(i + 1) + "<br>"
              "MAC: " + String(macStr) + "<br>"
              "RSSI: " + String(stationList.sta[i].rssi) + " dBm</li>";
    }
  } else {
    html += "<li>Fehler beim Abrufen der Stationen</li>";
  }

  html += "</ul>"
          "<p>Anzahl verbundener Geräte: " + String(WiFi.softAPgetStationNum()) + "</p>"
          "</body></html>";
          
  server.send(200, "text/html", html);
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
    server.handleClient();
  }

  delay(10);
}