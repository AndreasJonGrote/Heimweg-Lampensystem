/*
 * Master-Display.ino
 * 
 * Dieses Programm steuert ein ST7735 TFT-Display an und implementiert
 * grundlegende Textdarstellungsfunktionen mit Unterstützung für:
 * - Deutsche Umlaute (UTF-8 zu ASCII Konvertierung)
 * - Automatischer Zeilenumbruch
 * - Konfigurierbares Padding
 * - Verschiedene Textgrößen und Farben
 */

#include <Adafruit_GFX.h>    // Grafik-Bibliothek für verschiedene Displays
#include <Adafruit_ST7735.h> // Spezifische Bibliothek für ST7735 TFT-Displays
#include <SPI.h>             // SPI-Kommunikation für Display-Ansteuerung

// Definition der Display-Pins
#define TFT_CS     5        // Chip Select Pin
#define TFT_RST    4        // Reset Pin
#define TFT_DC     2        // Data/Command Pin

// Initialisierung des Display-Objekts
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Globale Layout-Einstellungen
const int padding = 10;     // Randabstand in Pixeln
int currentY = padding;     // Aktuelle Y-Position für Textausgabe

/**
 * Setzt die Y-Position für die Textausgabe zurück
 * Wird verwendet, um von vorne mit dem Schreiben zu beginnen
 */
void resetLines() {
  currentY = padding;
}

/**
 * Konvertiert deutsche Umlaute und Sonderzeichen in ASCII-Darstellung
 * @param text Der zu konvertierende Text
 * @return String mit ersetzten Umlauten
 */
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

/**
 * Zeichnet eine Textzeile mit automatischem Umbruch
 * @param text Der darzustellende Text
 * @param size Textgröße (Standard: 1)
 * @param color Textfarbe (Standard: Weiß)
 */
void drawWrappedLine(String text, int size = 1, uint16_t color = ST77XX_WHITE) {
  String textCopy = utf8ToAscii(text);

  // Berechnung der Textdimensionen
  int charWidth = 6 * size;                    // Breite eines Zeichens
  int maxLineWidth = tft.width() - 2 * padding; // Maximale Zeilenbreite
  int charsPerLine = maxLineWidth / charWidth;  // Zeichen pro Zeile
  int lineHeight = 8 * size + 2;               // Zeilenhöhe mit Abstand

  // Text in Zeilen aufteilen und ausgeben
  for (int i = 0; i < textCopy.length(); i += charsPerLine) {
    String line = textCopy.substring(i, min(i + charsPerLine, (int)textCopy.length()));
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setCursor(padding, currentY);
    tft.print(line);
    currentY += lineHeight;
  }
}

/**
 * Setup-Routine: Wird einmalig beim Start ausgeführt
 * Initialisiert das Display und zeigt Beispieltext an
 */
void setup() {
  Serial.begin(115200);               // Seriellen Monitor initialisieren
  
  // Display-Initialisierung
  tft.initR(INITR_BLACKTAB);         // Spezifische Display-Variante
  tft.setRotation(0);                // Portrait-Modus
  tft.fillScreen(ST77XX_BLACK);      // Bildschirm schwarz färben
  resetLines();                      // Textposition zurücksetzen

  // Beispieltext zum Testen der Funktionen
  drawWrappedLine("Padding links & rechts – passt!", 1);
  drawWrappedLine("Große Wörter wie Übersetzung oder äußert erscheinen korrekt.", 1);
  drawWrappedLine("Noch mehr Text in Rot, der umbricht.", 1);
}

/**
 * Hauptschleife: Wird kontinuierlich ausgeführt
 * Aktuell keine Funktionalität implementiert
 */
void loop() {
  // Keine Aktionen in der Hauptschleife
}