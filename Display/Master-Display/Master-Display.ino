#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// TFT-Pins
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Padding und Textlayout
const int padding = 10;
int currentY = padding;

void resetLines() {
  currentY = padding;
}

// UTF-8-Ersetzung
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

// Text zeichnen mit Padding, Umbruch und Farben
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

void setup() {
  Serial.begin(115200);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  resetLines();

  drawWrappedLine("Padding links & rechts – passt!", 1);
  drawWrappedLine("Große Wörter wie Übersetzung oder äußert erscheinen korrekt.", 1);
  drawWrappedLine("Noch mehr Text in Rot, der umbricht.", 1);
}

void loop() {
  // Nix hier
}