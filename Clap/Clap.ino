// ----------------------------------------------------
// Mikrofonmodul für ESP32 mit MAX9814
// Unterstützt:
// - Klatscherkennung mit Intensität
// - Gleitenden Lautstärkepegel (smoothLevel)
// - Langfristigen Raumpegel (15s Durchschnitt)
// Alle Werte normalisiert (0.0–1.0)
// ----------------------------------------------------

#define MIC_PIN 34  // ADC-Eingang für MAX9814

// === Einstellwerte ===
const int mic_sampleWindow = 30;                // Messdauer in ms
const unsigned long mic_cooldownTime = 1500;    // Mindestabstand zwischen Klatschern
const float mic_clapThresholdFactor = 0.9;      // Prozent vom MaxPeak für Erkennung
const int mic_impulseDelta = 400;               // Mindeständerung zur letzten Messung
const float mic_maxDecayFactor = 0.98;          // maxPeak sinkt langsam zurück

// === Zustand ===
int mic_maxPeak = 1000;
int mic_lastPeak = 0;
unsigned long mic_lastClapTime = 0;
unsigned long mic_lastSampleTime = 0;

// === Gleitender Mittelwert ===
const int mic_smoothBufferSize = 10;
float mic_smoothBuffer[mic_smoothBufferSize];
int mic_smoothIndex = 0;
float mic_smoothSum = 0;
float mic_smoothLevel = 0.0;

// === 15s Durchschnittspegel ===
unsigned long mic_avgStartTime = 0;
unsigned long mic_avgSamples = 0;
unsigned long mic_avgSum = 0;
const unsigned long mic_averageWindow = 15000;

// === ClapEvent-Struktur ===
struct mic_ClapEvent {
  bool triggered;     // true, wenn Klatscher erkannt
  float intensity;    // normalisierter Peak (0.0–1.0)
};

mic_ClapEvent mic_lastClap = { false, 0.0 };

// === SETUP ===
void setup() {
  Serial.begin(115200);
  mic_avgStartTime = millis();
  mic_lastSampleTime = millis();
  Serial.println("🎤 mic_update() aktiv – modular & robust");
}

// === LOOP ===
void loop() {
  mic_update();  // Aufruf im Hauptloop immer nötig
}

// === Messung & Verarbeitung ===
void mic_update() {
  unsigned long currentTime = millis();

  if (currentTime - mic_lastSampleTime >= mic_sampleWindow) {
    mic_lastSampleTime = currentTime;

    // --- Einzelmessung durchführen ---
    int signalMin = 4095;
    int signalMax = 0;
    long signalSum = 0;
    int samples = 0;

    unsigned long start = millis();
    while (millis() - start < mic_sampleWindow) {
      int sample = analogRead(MIC_PIN);
      signalSum += sample;
      if (sample < signalMin) signalMin = sample;
      if (sample > signalMax) signalMax = sample;
      samples++;
    }

    int peakToPeak = signalMax - signalMin;
    float avg = (float)signalSum / samples;

    if (peakToPeak > mic_maxPeak) mic_maxPeak = peakToPeak;
    mic_maxPeak *= mic_maxDecayFactor;
    if (mic_maxPeak < 300) mic_maxPeak = 300;

    float normalizedPeak = (float)peakToPeak / mic_maxPeak;
    normalizedPeak = constrain(normalizedPeak, 0.0, 1.0);

    // --- Gleitender Smooth-Level aktualisieren ---
    mic_smoothSum -= mic_smoothBuffer[mic_smoothIndex];
    mic_smoothBuffer[mic_smoothIndex] = normalizedPeak;
    mic_smoothSum += normalizedPeak;
    mic_smoothIndex = (mic_smoothIndex + 1) % mic_smoothBufferSize;
    mic_smoothLevel = mic_smoothSum / mic_smoothBufferSize;

    // --- 15s Raumpegel (PeakSum) ---
    mic_avgSum += peakToPeak;
    mic_avgSamples++;

    if (millis() - mic_avgStartTime >= mic_averageWindow) {
      float avg15s = (float)mic_avgSum / mic_avgSamples;
      Serial.print("🌡 Raumpegel (15s): ");
      Serial.println((int)avg15s);
      mic_avgStartTime = millis();
      mic_avgSum = 0;
      mic_avgSamples = 0;
    }

    // --- Klatscherkennung ---
    int delta = peakToPeak - mic_lastPeak;
    mic_lastPeak = peakToPeak;

    mic_lastClap.triggered = false;
    mic_lastClap.intensity = 0.0;

    if (
      peakToPeak > (int)(mic_maxPeak * mic_clapThresholdFactor) &&
      delta > mic_impulseDelta &&
      millis() - mic_lastClapTime > mic_cooldownTime
    ) {
      mic_lastClap.triggered = true;
      mic_lastClap.intensity = normalizedPeak;
      mic_lastClapTime = millis();
    }
  }
}

// === Öffentliche Schnittstelle ===

/**
 * Gibt den aktuellen Smooth-Lautstärkepegel (0.0–1.0) zurück.
 * Ideal für Helligkeitssteuerung im Modus "smooth".
 */
float mic_getSmoothLevel() {
  return mic_smoothLevel;
}

/**
 * Gibt zurück, ob ein Klatscher erkannt wurde und wie stark er war.
 * - triggered: true, wenn Klatscher im aktuellen Fenster erkannt
 * - intensity: 0.0–1.0 normalisiert, wie laut der Klatscher war
 */
mic_ClapEvent mic_getClapEvent() {
  return mic_lastClap;
}

/**
 * Gibt den Durchschnittspegel der letzten 15 Sekunden (normalisiert).
 * Ideal zur Anzeige, Diagnose oder als dynamische Referenz.
 */
float mic_getAvgLevel15s() {
  float avg = (mic_avgSamples > 0) ? ((float)mic_avgSum / mic_avgSamples) : 0.0;
  return constrain(avg / mic_maxPeak, 0.0, 1.0);
}