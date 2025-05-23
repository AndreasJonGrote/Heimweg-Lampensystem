#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>

// TFT Pins
#define TFT_CS   5
#define TFT_RST  4
#define TFT_DC   2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Layout
const int padding = 10;
int currentY = padding;

// Access Point Einstellungen
const char* ssid = "TestDisplay";
const char* password = "12345678";
const int channel = 6;  // Stabiler auf Apple-Geräten
const bool hide_ssid = false;
const int max_connection = 4;

IPAddress localIP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

// Webserver
WebServer server(80);

// UTF-8 zu ASCII für Display
String utf8ToAscii(String text) {
  text.replace("ä", "ae");
  text.replace("ö", "oe");
  text.replace("ü", "ue");
  text.replace("Ä", "Ae");
  text.replace("Ö", "Oe");
  text.replace("Ü", "Ue");
  text.replace("ß", "ss");
  text.replace("–", "-");
  text.replace("—", "-");
  return text;
}

// Textausgabe mit Umbruch
void resetLines() {
  currentY = padding;
}

void drawWrappedLine(String text, int size = 1, uint16_t color = ST77XX_WHITE) {
  String textCopy = utf8ToAscii(text);
  int charWidth = 6 * size;
  int maxLineWidth = tft.width() - 2 * padding;
  int charsPerLine = maxLineWidth / charWidth;
  int lineHeight = 8 * size + 2;

  for (int i = 0; i < textCopy.length(); i += charsPerLine) {
    String line = textCopy.substring(i, min(i + charsPerLine, (int)textCopy.length()));
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setCursor(padding, currentY);
    tft.print(line);
    currentY += lineHeight;
  }
}

// Webserver-Handler
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><meta charset='UTF-8'><title>Display</title></head>
    <body style='font-family:sans-serif;text-align:center;padding-top:40px'>
      <h1>ESP32 Display</h1>
      <p>Access Point ist aktiv.</p>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// Setup Webserver
void setupWebServer() {
  server.on("/", handleRoot);
  server.begin();
}

// Setup WiFi
bool setupWiFiAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  WiFi.mode(WIFI_AP);
  delay(100);

  bool success = WiFi.softAP(ssid, password, channel, hide_ssid, max_connection);
  if (!success) return false;

  if (!WiFi.softAPConfig(localIP, gateway, subnet)) return false;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  resetLines();
  drawWrappedLine("Starte WiFi Access Point...", 1, ST77XX_YELLOW);

  if (!setupWiFiAP()) {
    drawWrappedLine("WLAN Fehler!", 2, ST77XX_RED);
    return;
  }

  drawWrappedLine("WLAN Access Point bereit.", 1, ST77XX_GREEN);
  drawWrappedLine(WiFi.softAPIP().toString(), 1);

  setupWebServer();
  drawWrappedLine("Webserver gestartet", 1, ST77XX_GREEN);

  Serial.println("Fertig!");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  static unsigned long lastCheck = 0;
  server.handleClient();

  if (millis() - lastCheck > 10000) {
    int connected = WiFi.softAPgetStationNum();
    Serial.printf("Clients: %d\n", connected);

    tft.fillRect(0, tft.height()-20, tft.width(), 20, ST77XX_BLACK);
    tft.setCursor(padding, tft.height()-15);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Verbunden: ");
    tft.print(connected);

    lastCheck = millis();
  }
}