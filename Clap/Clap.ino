// Mikrofon-Pin (OUT des MAX9814 an GPIO 34)
#define MIC_PIN 34

// Länge des Messfensters (wie lange Pegel gesammelt werden, in Millisekunden)
const int sampleWindow = 30;

// Schwellenwerte für Klatscherkennung
const int DROP_THRESHOLD = 600;       // Der Signalwert muss nach dem Peak um mindestens diesen Wert abfallen
const int CLAP_COOLDOWN = 400;        // Zeit in ms, wie lange nach einem Klatscher keine weitere Erkennung erfolgen soll

// Raumpegel-Mittelwert alle 15 Sekunden berechnen
const unsigned long AVERAGE_DURATION = 15000;

// Zustände der Klatscherkennung
enum ClapState { IDLE, PEAK_DETECTED };
ClapState state = IDLE;

int lastPeak = 0;                     // Speichert den letzten erkannten Spitzenwert
int maxPeak = 1000;                   // Dynamischer Maximalwert, an dem sich die Klatscherkennung orientiert
float clapThresholdFactor = 0.90;     // Klatscher müssen 90 % oder mehr des höchsten bisherigen Peaks haben

unsigned long lastClapTime = 0;       // Zeitpunkt des letzten Klatschereignisses
unsigned long averageStartTime = 0;   // Startzeitpunkt des aktuellen 15s-Durchschnittsintervalls
unsigned long averageSamples = 0;     // Anzahl gemessener Samples im aktuellen Intervall
unsigned long averageSum = 0;         // Summe aller Peaks im Intervall zur Mittelwertbildung

void setup() {
  Serial.begin(115200);               // Serielle Ausgabe starten
  averageStartTime = millis();        // Startzeit für Mittelwert setzen
  Serial.println("--- Mikrofonanalyse gestartet (dynamische Schwelle) ---");
}

void loop() {
  // ➤ 1. Kurzzeit-Messung der Lautstärke (Peak-to-Peak)
  unsigned long startMillis = millis();
  int signalMin = 4095;               // Startwert für Minimum (höchstmöglicher ADC-Wert)
  int signalMax = 0;                  // Startwert für Maximum

  // Sammle in kurzer Zeit den Maximal- und Minimalwert des Signals
  while (millis() - startMillis < sampleWindow) {
    int sample = analogRead(MIC_PIN);  // Lese Rohwert vom Mikrofon
    if (sample > signalMax) signalMax = sample;
    if (sample < signalMin) signalMin = sample;
  }

  int peakToPeak = signalMax - signalMin;  // Differenz = Lautstärke/Amplitude

  // ➤ 2. Automatische Anpassung des Maximalwerts
  // Wenn der aktuelle Wert höher ist als alle bisher gemessenen, aktualisiere den Maximalwert
  if (peakToPeak > maxPeak) {
    maxPeak = peakToPeak;
    Serial.print("⚙️ Neuer Maximalwert erkannt: ");
    Serial.println(maxPeak);
  }

  // Schwelle für Klatscherkennung auf Basis des höchsten Peaks
  int dynamicThreshold = (int)(maxPeak * clapThresholdFactor);

  // ➤ 3. Klatscherkennung per Zustandsmaschine
  switch (state) {
    case IDLE:
      // Wenn Lautstärke über Schwelle liegt und genug Zeit seit letztem Klatscher vergangen ist
      if (peakToPeak > dynamicThreshold && millis() - lastClapTime > CLAP_COOLDOWN) {
        state = PEAK_DETECTED;
        lastPeak = peakToPeak;
      }
      break;

    case PEAK_DETECTED:
      // Wenn Lautstärke deutlich abfällt → echter Klatscher
      if (peakToPeak < lastPeak - DROP_THRESHOLD) {
        Serial.print("👏 KLATSCH erkannt!  Peak: ");
        Serial.print(lastPeak);
        Serial.print(" | Schwelle: ");
        Serial.println(dynamicThreshold);
        lastClapTime = millis();
        state = IDLE;
      }
      // Sicherheitsrückkehr nach 100 ms (z. B. bei Störsignal)
      if (millis() - startMillis > 100) {
        state = IDLE;
      }
      break;
  }

  // ➤ 4. Sammle Daten für Mittelwert
  averageSum += peakToPeak;
  averageSamples++;

  // ➤ 5. Zeige alle 15 Sekunden den durchschnittlichen Raumpegel
  if (millis() - averageStartTime > AVERAGE_DURATION) {
    float avg = (float)averageSum / averageSamples;
    Serial.print("🌡 Durchschnittsraumpegel (15s): ");
    Serial.println((int)avg);
    averageStartTime = millis();
    averageSum = 0;
    averageSamples = 0;
  }

  delay(10);  // kurze Pause für bessere Reaktionszeit
}