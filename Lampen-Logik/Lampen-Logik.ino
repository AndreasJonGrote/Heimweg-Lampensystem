/**
 * Heimweg-Lampensystem – Bewegungsaktivierte LED-Beleuchtung
 * Version 1.0
 * 
 * Hardware-Spezifikationen:
 * --------------------------
 * - Mikrocontroller: ESP32 Dev Board
 * - Ultraschallsensor: HC-SR04
 * - LED-Ring: WS2812B (NeoPixel), 12 oder 16 LEDs (konfigurierbar)
 * - Stromversorgung: 5V (mind. 1A empfohlen bei 16 LEDs)
 * 
 * Features:
 * ---------
 * - Sanftes Fade-in/out mit Sinus-Easing
 * - Ultraschall-Bewegungserkennung mit Mittelwert-Glättung
 * - Bootanimation (LED-Test & Statussignal)
 * - Anpassbare LED-Anzahl
 * 
 * Pinbelegung:
 * ------------
 * LED_PIN:  4  - Datenleitung für den NeoPixel LED-Ring
 * TRIG_PIN: 5  - Trigger-Pin für HC-SR04 Sensor
 * ECHO_PIN: 18 - Echo-Pin für HC-SR04 Sensor
 */

// ==================== BIBLIOTHEKEN ====================
#include <Adafruit_NeoPixel.h>

// ==================== PIN-DEFINITIONEN ====================
#define LED_PIN     4
#define LED_COUNT   16
#define TRIG_PIN    21
#define ECHO_PIN    18

// ==================== OBJEKT-INITIALISIERUNG ====================
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ==================== KONFIGURATION ====================
// Bewegungserkennung
float triggerMinDistance = 10.0;    // Minimale Erkennungsdistanz in cm
float triggerMaxDistance = 150.0;   // Maximale Erkennungsdistanz in cm

// Timing-Einstellungen
int fadeInTime = 250;      // Dauer des Einblendens in ms
int fadeOutTime = 1250;    // Dauer des Ausblendens in ms
int holdOnTime = 100;      // Nachleuchtzeit nach letzter Bewegung in ms
const int frameInterval = 20;  // Aktualisierungsintervall (50 FPS)

// Lichteinstellungen
const uint8_t COLOR_SETTING[] = {
  255,  // R: Rot-Komponente
  159,  // G: Grün-Komponente
  102   // B: Blau-Komponente
};

// ==================== GLOBALE VARIABLEN ====================
// Zustandsvariablen
bool triggered = false;     // Aktuelle Bewegungserkennung
bool fadeInRunning = false; // Einblend-Animation aktiv
bool lightOn = false;      // LEDs sind an
float brightness = 0.0;     // Aktuelle Helligkeit (0.0 - 1.0)

// Zeitsteuerung
unsigned long lastUpdate = 0;       // Letztes LED-Update
unsigned long triggerTimeout = 0;   // Timeout für Messungen
unsigned long lastTriggerTime = 0;  // Letzte Bewegungserkennung

// Messwertspeicher
float distanceHistory[5];  // Speicher für Mittelwertbildung

// ==================== SETUP ====================
void setup() {
  // Hardware-Initialisierung
  strip.begin();
  strip.show();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.begin(115200);

  // Initialisiere Distance History mit sicheren Startwerten
  for(int i = 0; i < 5; i++) {
    distanceHistory[i] = 999.0;  // Außerhalb des Erkennungsbereichs
  }

  // Boot-Sequenz und Reset
  ledBootCheck();  // Boot-Animation

  // Reset aller Zustände nach Boot-Animation
  triggered = false;
  fadeInRunning = false;
  lightOn = false;
  brightness = 0.0;
  lastUpdate = millis();
  triggerTimeout = millis();
  lastTriggerTime = millis();
}

// ==================== HAUPTSCHLEIFE ====================
void loop() {
  unsigned long currentMillis = millis();

  // Bewegungserkennung (alle 100ms)
  if (currentMillis - triggerTimeout > 100) {
    triggerTimeout = currentMillis;
    float distance = getDistanceAverage(5);
    Serial.print("Entfernung (geglättet): ");
    Serial.print(distance);
    Serial.println(" cm");

    // Prüfe ob Bewegung im gültigen Bereich
    bool inRange = (distance >= triggerMinDistance && distance <= triggerMaxDistance);

    if (inRange) {
      triggered = true;
      lastTriggerTime = currentMillis;
      if (brightness < 1.0 && !fadeInRunning) {
        fadeInRunning = true;
      }
      lightOn = true;
    } else {
      if (holdOnTime == 0 || (currentMillis - lastTriggerTime) > holdOnTime) {
        triggered = false;
      }
    }
  }

  // LED-Steuerung (framebasiert)
  if (currentMillis - lastUpdate >= frameInterval) {
    lastUpdate = currentMillis;

    float stepIn = frameInterval / (float)fadeInTime;
    float stepOut = frameInterval / (float)fadeOutTime;

    // Helligkeitssteuerung
    if (fadeInRunning) {
      brightness += stepIn;
      if (brightness >= 1.0) {
        brightness = 1.0;
        fadeInRunning = false;
      }
    } else if (triggered) {
      brightness += stepIn;
      if (brightness > 1.0) brightness = 1.0;
    } else {
      brightness -= stepOut;
      if (brightness < 0.0) brightness = 0.0;
    }

    setAllLEDs(brightness);
  }
}

// ==================== SENSORFUNKTIONEN ====================
/**
 * Berechnet den Durchschnitt mehrerer Messungen
 * @param samples Anzahl der Messungen für Durchschnittsberechnung
 * @return Geglätteter Distanzwert in cm
 */
float getDistanceAverage(int samples) {
  float sum = 0;
  for (int i = samples - 1; i > 0; i--) {
    distanceHistory[i] = distanceHistory[i - 1];
    sum += distanceHistory[i];
  }
  float newDist = getDistance();
  distanceHistory[0] = newDist;
  sum += newDist;
  return sum / samples;
}

/**
 * Einzelne Distanzmessung mit HC-SR04
 * @return Gemessene Distanz in cm
 */
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;  // Kein Echo = Maximaldistanz
  return duration * 0.0343 / 2;     // Umrechnung in cm
}

// ==================== LED-FUNKTIONEN ====================
/**
 * Berechnet eine sanfte Überblendung mittels Sinus
 * @param t Wert zwischen 0.0 und 1.0
 * @return Geglätteter Wert zwischen 0.0 und 1.0
 */
float easeInOut(float t) {
  return 0.5 * (1 - cos(t * PI));
}

/**
 * Setzt alle LEDs auf eine bestimmte Helligkeit
 * @param b Helligkeit zwischen 0.0 und 1.0
 */
void setAllLEDs(float b) {
  float eased = easeInOut(constrain(b, 0.0, 1.0));
  
  // Farbwerte aus Konfiguration anwenden
  uint8_t r = COLOR_SETTING[0] * eased;
  uint8_t g = COLOR_SETTING[1] * eased;
  uint8_t b2 = COLOR_SETTING[2] * eased;

  uint32_t color = strip.Color(r, g, b2);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

/**
 * Führt die Boot-Animation durch
 * - Rotes Licht läuft einmal um den Ring
 * - Grünes Aufleuchten als Erfolgsindikator
 */
void ledBootCheck() {
  int fullOnTime = 250;
  int fadeTime = fullOnTime / 2;

  // Roter Laufeffekt
  for (int i = 0; i < LED_COUNT; i++) {
    for (int b = 0; b <= 255; b += 15) {
      strip.clear();
      strip.setPixelColor(i, strip.Color(b, 0, 0));
      if (i > 0) {
        int prev = i - 1;
        int prevBrightness = 255 - b;
        strip.setPixelColor(prev, strip.Color(prevBrightness, 0, 0));
      }
      strip.show();
      delay(fadeTime / (255 / 15));
    }
  }

  // Grünes Erfolgssignal
  for (int b = 0; b <= 255; b += 5) {
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, b, 0));
    }
    strip.show();
    delay(1000 / (255 / 5));  // ca. 1000ms Fading
  }

  delay(1000);  // Grün voll sichtbar

  strip.clear();
  strip.show();
}
