// AR on GND, Gain on 5V
#define MIC_PIN 34  // ADC-Eingang für MAX9814 (AR auf GND, Gain auf 5V)

// Konstanten für die Klatscherkennung
const int mic_CLAP_THRESHOLD = 900;      // Grundempfindlichkeit: Ab diesem Amplitudenwert wird ein Signal als möglicher Klatscher erkannt
const int mic_MIN_TIME_BETWEEN_CLAPS = 300;  // Minimale Zeit (ms) zwischen zwei Klatschern, verhindert Mehrfacherkennung
const int mic_NOISE_FLOOR = 150;         // Rauschunterdrückung: Signale unter diesem Wert werden ignoriert
const int mic_MAX_CLAP_DURATION = 50;    // Maximale Dauer (ms) eines Klatschers, filtert längere Geräusche
const float mic_MIN_INTENSITY = 0.20;    // Minimale normalisierte Intensität (0.0-1.0) für gültige Klatscher
                                        // Berechnung: (Amplitude-THRESHOLD)/THRESHOLD, also hier min. 20% über THRESHOLD
const int mic_DOUBLE_CLAP_LOCKTIME = 5000;  // Sperrzeit (ms) nach erkanntem Doppelklatscher
const unsigned long mic_SAMPLE_INTERVAL = 1; // Abtastintervall (ms) für die Signalverarbeitung
const unsigned long mic_RECALIBRATION_INTERVAL = 300000; // Rekalibrierung alle 5 Minuten (300000ms)

// Konfiguration
bool mic_autoRecalibrate = true;  // Automatische Rekalibrierung alle 5 Minuten

// Globale Zustandsvariablen
int mic_baseline = 0;            // Grundpegel des Mikrofons, wird in setup() kalibriert
                                // Wird von allen Amplitudenmessungen abgezogen, um den "echten" Ausschlag zu messen
unsigned long mic_lastClapTime = 0;  // Zeitpunkt des letzten Klatschens
int mic_clapCount = 0;          // Zählt Klatscher
unsigned long mic_firstClapTime = 0;    // Zeitpunkt des ersten Klatschens
bool mic_isLocked = false;      // Sperrstatus
unsigned long mic_lockUntil = 0;        // Sperrzeit bis zu diesem Zeitpunkt
unsigned long mic_lastSampleTime = 0;  // Zeitpunkt der letzten Messung
unsigned long mic_lastCalibrationTime = 0; // Zeitpunkt der letzten Kalibrierung
bool mic_isPotentialClap = false;  // Flag für mögliches Klatschen
int mic_maxAmplitude = 0;       // Maximale Amplitude während eines Events
unsigned long mic_eventStartTime = 0; // Startzeit eines Events

void setup() {
  Serial.begin(115200);
  
  // ACHTUNG: Diese Einstellung ist global für den gesamten ESP32!
  // Wenn andere Skripte analogRead() verwenden, müssen sie mit 12-Bit-Werten (0-4095) arbeiten.
  // Standard wäre 10-Bit (0-1023). Schwellwerte in anderen Skripten müssen ggf. angepasst werden.
  analogReadResolution(12); // 12-bit Auflösung (0-4095)
  
  Serial.println("Kalibriere...");
  
  mic_calibrateBaseline();
  
  Serial.print("Baseline: ");
  Serial.println(mic_baseline);
  Serial.println("-------------------");
  if (mic_autoRecalibrate) {
    Serial.println("Automatische Rekalibrierung aktiv (alle 5 Minuten)");
    Serial.println("-------------------");
  }
}

void mic_calibrateBaseline() {
  // Kalibriert den Grundpegel (Baseline) des Mikrofons
  // - Nimmt 100 Samples über 1 Sekunde
  // - Berechnet den Durchschnitt als Referenzwert
  // - Dieser Wert wird später von allen Messungen abgezogen,
  //   um den tatsächlichen Ausschlag des Signals zu ermitteln
  // - Wichtig: Während der Kalibrierung sollte es möglichst leise sein!
  
  long sum = 0;
  for(int i = 0; i < 100; i++) {
    sum += analogRead(MIC_PIN);
    delay(10);  // 100 Samples über 1 Sekunde verteilt
  }
  int newBaseline = sum / 100;
  
  // Optional: Große Änderungen in der Console ausgeben
  if (abs(newBaseline - mic_baseline) > 100) {
    Serial.println("------------------------");
    Serial.print("Baseline angepasst: ");
    Serial.print(mic_baseline);
    Serial.print(" -> ");
    Serial.println(newBaseline);
    Serial.println("------------------------");
  }
  
  mic_baseline = newBaseline;
  mic_lastCalibrationTime = millis();
}

void mic_checkLockStatus() {
  unsigned long currentTime = millis();
  if (mic_isLocked && currentTime >= mic_lockUntil) {
    mic_isLocked = false;
    mic_clapCount = 0;
  }
}

void mic_processClap(float intensity, unsigned long currentTime) {
  mic_clapCount++;
  if (mic_clapCount == 1) {
    mic_firstClapTime = currentTime;
  }
  else if (mic_clapCount == 2 && (currentTime - mic_firstClapTime) < 1000) {
    Serial.println("------------------------");
    Serial.println("Doppelklatscher! Sperre aktiv für 5 Sekunden");
    Serial.println("------------------------");
    mic_isLocked = true;
    mic_lockUntil = currentTime + mic_DOUBLE_CLAP_LOCKTIME;
    mic_clapCount = 0;
  }
  else if (currentTime - mic_firstClapTime > 1000) {
    mic_clapCount = 1;
    mic_firstClapTime = currentTime;
  }
  
  if (!mic_isLocked && mic_clapCount == 1) {
    // ====================================================================
    // HIER: Verarbeitung des normalisierten Intensitätswerts (0.0 - 1.0)
    // 
    // Der Intensitätswert ist bereits:
    // - Normalisiert zwischen 0.0 und 1.0
    // - Gefiltert (nur gültige Klatscher über CLAP_THRESHOLD)
    // - Über MIN_INTENSITY (mind. 0.20)
    // - Zeitlich gefiltert (MAX_CLAP_DURATION, MIN_TIME_BETWEEN_CLAPS)
    //
    // Beispiel für Weiterverarbeitung:
    // void processIntensity(float value) {
    //   // value ist normalisiert zwischen 0.0 und 1.0
    //   // z.B. LED-Helligkeit = value * 255
    //   // oder andere intensitätsbasierte Aktionen
    // }
    // ====================================================================
    
    Serial.println("------------------------");
    Serial.print("KLATSCHEN! Intensität: ");
    Serial.print(intensity, 2);
    Serial.println("");
    Serial.println("------------------------");
  }
  
  mic_lastClapTime = currentTime;
}

void mic_processAudio() {
  int mic_rawValue = analogRead(MIC_PIN);
  int mic_amplitude = abs(mic_rawValue - mic_baseline);
  unsigned long currentTime = millis();
  
  if (!mic_isPotentialClap && mic_amplitude > mic_NOISE_FLOOR) {
    mic_isPotentialClap = true;
    mic_eventStartTime = currentTime;
    mic_maxAmplitude = mic_amplitude;
  }
  else if (mic_isPotentialClap) {
    if (mic_amplitude > mic_maxAmplitude) {
      mic_maxAmplitude = mic_amplitude;
    }
    
    if (mic_amplitude < mic_NOISE_FLOOR || (currentTime - mic_eventStartTime) > mic_MAX_CLAP_DURATION) {
      unsigned long mic_eventDuration = currentTime - mic_eventStartTime;
      
      float mic_intensity = float(mic_maxAmplitude - mic_CLAP_THRESHOLD) / float(mic_CLAP_THRESHOLD);
      mic_intensity = constrain(mic_intensity, 0.0, 1.0);
      
      if (mic_maxAmplitude > mic_CLAP_THRESHOLD && 
          mic_eventDuration < mic_MAX_CLAP_DURATION && 
          currentTime - mic_lastClapTime > mic_MIN_TIME_BETWEEN_CLAPS &&
          mic_intensity >= mic_MIN_INTENSITY &&
          !mic_isLocked) {
        
        mic_processClap(mic_intensity, currentTime);
      }
      
      mic_isPotentialClap = false;
      mic_maxAmplitude = 0;
    }
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Prüfe ob Rekalibrierung nötig ist
  if (mic_autoRecalibrate && currentTime - mic_lastCalibrationTime >= mic_RECALIBRATION_INTERVAL) {
    // Nur rekalibrieren wenn gerade kein Klatscher verarbeitet wird
    if (!mic_isPotentialClap && !mic_isLocked) {
      mic_calibrateBaseline();
    }
  }
  
  if (currentTime - mic_lastSampleTime >= mic_SAMPLE_INTERVAL) {
    mic_lastSampleTime = currentTime;
    mic_checkLockStatus();
    mic_processAudio();
  }
}