/**
 * Version 0.1a
 * @todo Stabilität im Ultraschall
 * @todo Holdtime vs. Threshold wegen Framedrops
 */

// Bibliothek für die Steuerung der NeoPixel LEDs einbinden
#include <Adafruit_NeoPixel.h>

// Pin-Definitionen für Hardware-Komponenten
#define LED_PIN 4        // Digitaler Pin für die LED-Streifen-Steuerung
#define LED_COUNT 12     // Anzahl der LEDs im Streifen

// Pins für den Ultraschallsensor HC-SR04
#define TRIG_PIN 5       // Trigger-Pin für den Ultraschallsensor
#define ECHO_PIN 18      // Echo-Pin für den Ultraschallsensor

// Initialisierung des LED-Streifens mit 12 LEDs, Pin 4, RGB-Farbmodell und 800KHz
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Konfiguration der Dimming-Parameter
int fadeInTime = 250;    // Zeit in ms für das Hochdimmen der LEDs
int fadeOutTime = 1250;  // Zeit in ms für das Ausdimmen der LEDs
bool dynamicFadeIn = false;  // Steuerung des Dimming-Verhaltens: false = vollständiges Hochdimmen, true = dynamisches Dimmen

// Konfiguration der Nachleuchtzeit
int holdOnTime = 50;      // Zeit in ms, die die LEDs nach dem letzten Trigger anbleiben (0 = sofort aus)

// Konfiguration des Erfassungsbereichs des Ultraschallsensors
float triggerMinDistance = 10.0;   // Minimale Distanz in cm für die Aktivierung
float triggerMaxDistance = 150.0;  // Maximale Distanz in cm für die Aktivierung

// Konfiguration der Dimming-Kurve
bool useEasing = true;   // Aktiviert sanftes Dimmen durch quadratische Kurve (Ease-Out)

// Zustandsvariablen für die Steuerungslogik
bool triggered = false;      // Gibt an, ob der Sensor eine Bewegung erfasst hat
bool lightOn = false;        // Gibt an, ob die LEDs eingeschaltet sind
bool fadeInRunning = false;  // Gibt an, ob ein FadeIn-Prozess läuft

// Helligkeitswert zwischen 0.0 (aus) und 1.0 (voll)
float brightness = 0.0;

// Zeitsteuerungsvariablen
unsigned long lastUpdate = 0;        // Zeitpunkt der letzten LED-Aktualisierung
unsigned long triggerTimeout = 0;    // Zeitpunkt des letzten Sensor-Triggers
unsigned long lastTriggerTime = 0;   // Zeitpunkt der letzten Bewegungserfassung
int holdTime = 100;                  // Intervall zwischen Ultraschallmessungen in ms

// Initialisierungsfunktion
void setup() {
  strip.begin();                    // LED-Streifen initialisieren
  strip.show();                     // Alle LEDs aus
  pinMode(TRIG_PIN, OUTPUT);        // Trigger-Pin als Ausgang konfigurieren
  pinMode(ECHO_PIN, INPUT);         // Echo-Pin als Eingang konfigurieren
  Serial.begin(115200);             // Serielle Kommunikation für Debugging starten
}

// Hauptschleife
void loop() {
  unsigned long currentMillis = millis();

  // Ultraschallmessung im definierten Intervall
  if (currentMillis - triggerTimeout > holdTime) {
    triggerTimeout = currentMillis;
    float distance = getDistance();
    Serial.print("Entfernung: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Prüfen, ob die gemessene Distanz im aktivierenden Bereich liegt
    bool inRange = (distance >= triggerMinDistance && distance <= triggerMaxDistance);

    if (inRange) {
      triggered = true;
      lastTriggerTime = currentMillis;

      // FadeIn-Prozess starten, wenn nötig
      if (!dynamicFadeIn && brightness < 1.0 && !fadeInRunning) {
        fadeInRunning = true;
      }

      lightOn = true;
    } else {
      // Trigger zurücksetzen, wenn keine Bewegung mehr erfasst wird
      if (holdOnTime == 0 || (currentMillis - lastTriggerTime) > holdOnTime) {
        triggered = false;
      }
    }
  }

  // LED-Helligkeit im definierten Intervall aktualisieren
  if (currentMillis - lastUpdate > 20) {
    lastUpdate = currentMillis;

    if (fadeInRunning) {
      // Statisches Hochdimmen bis zum Maximum
      brightness += (20.0 / fadeInTime);
      if (brightness >= 1.0) {
        brightness = 1.0;
        fadeInRunning = false;
      }
    } else if (triggered) {
      if (dynamicFadeIn) {
        // Dynamisches Hochdimmen mit Unterbrechungsmöglichkeit
        brightness += (20.0 / fadeInTime);
        if (brightness > 1.0) brightness = 1.0;
      }
    } else {
      // Ausdimmen mit definierter Geschwindigkeit
      brightness -= (20.0 / fadeOutTime);
      if (brightness < 0.0) brightness = 0.0;
    }

    // Aktualisierte Helligkeit auf LEDs anwenden
    setAllLEDs(brightness);
  }
}

// Funktion zur Messung der Distanz mit dem Ultraschallsensor
float getDistance() {
  // Trigger-Impuls senden
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Echo-Zeit messen und in Distanz umrechnen
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distanceCm = duration * 0.0343 / 2;
  return distanceCm;
}

// Funktion zum Setzen der LED-Helligkeit mit Easing
void setAllLEDs(float b) {
  // Easing-Kurve anwenden für sanftere Übergänge
  float easedBrightness;
  if (useEasing) {
    easedBrightness = b * b;  // Quadratische Kurve für weichere Übergänge
    // Alternativ noch weicher: easedBrightness = b * b * b;
  } else {
    easedBrightness = b;  // Lineare Kurve ohne Easing
  }

  // Warmweiße Farbe berechnen
  uint8_t r = 255 * easedBrightness;  // Rot-Komponente
  uint8_t g = 200 * easedBrightness;  // Grün-Komponente
  uint8_t b2 = 150 * easedBrightness; // Blau-Komponente
  uint32_t color = strip.Color(r, g, b2);

  // Farbe auf alle LEDs anwenden
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}