/*
 * ESP32 Klatsch-Erkennungssystem mit MAX9814 Mikrofon
 * ================================================
 * 
 * HARDWARE SETUP:
 * --------------
 * MAX9814 Mikrofon:
 * - AR (Attack/Release) auf GND → schnelle Reaktion auf Amplitudenänderungen
 * - Gain auf 5V → maximale Verstärkung (60dB)
 * - Out an GPIO34 (ADC1_CH6) → Einer der zwei ADC-Pins die im WiFi-Modus nutzbar sind
 * 
 * ADC KONFIGURATION:
 * ----------------
 * - 12-bit Auflösung: 0-4095
 * - Sampling Rate: ~1ms (definiert durch mic_SAMPLE_INTERVAL)
 * - DC-Offset wird durch Baseline-Kalibrierung kompensiert
 * 
 * SIGNALVERARBEITUNG:
 * -----------------
 * 1. Baseline-Ermittlung:
 *    - 100 Samples über 1 Sekunde
 *    - Durchschnitt = Ruhelevel des Mikrofons
 *    - Automatische Rekalibrierung alle 5 Minuten (optional)
 * 
 * 2. Amplitudenberechnung:
 *    - Rohdaten - Baseline = tatsächliche Amplitude
 *    - Absoluter Wert (negative Ausschläge werden positiv)
 * 
 * 3. Klatsch-Erkennung:
 *    a) Vorfilterung:
 *       - Signal muss über NOISE_FLOOR (150) liegen
 *       - Aktiviert Klatsch-Erkennungsmodus
 * 
 *    b) Amplituden-Tracking:
 *       - Maximale Amplitude während des Events wird gespeichert
 *       - Event endet wenn Signal unter NOISE_FLOOR fällt
 *       - Oder MAX_CLAP_DURATION (50ms) überschritten wird
 * 
 *    c) Validierung:
 *       - Amplitude > CLAP_THRESHOLD (900)
 *       - Dauer < MAX_CLAP_DURATION (50ms)
 *       - Mindestabstand zum letzten Klatscher > MIN_TIME_BETWEEN_CLAPS (300ms)
 *       - Normalisierte Intensität >= MIN_INTENSITY (0.20)
 *       - Keine aktive Sperre
 * 
 * 4. Intensitätsberechnung:
 *    - Normalisierter Wert zwischen 0.0 und 1.0
 *    - Formel: (maxAmplitude - THRESHOLD) / THRESHOLD
 *    - Beispiel: 
 *      Bei THRESHOLD=900:
 *      900 → 0.0
 *      1800 → 1.0
 *      1350 → 0.5
 * 
 * DOPPELKLATSCHER:
 * --------------
 * - Zwei Klatscher innerhalb von 1 Sekunde = Doppelklatscher
 * - Löst 5-Sekunden-Sperre aus (DOUBLE_CLAP_LOCKTIME)
 * - Verhindert unbeabsichtigte weitere Erkennungen
 * 
 * AUTOMATISCHE REKALIBRIERUNG:
 * -------------------------
 * - Alle 5 Minuten (wenn aktiviert)
 * - Nur wenn gerade kein Klatscher erkannt wird
 * - Kompensiert:
 *   → Temperaturänderungen
 *   → Änderungen der Umgebungsgeräusche
 *   → Mikrofonalterung
 */

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

/*
 * GLOBALE ZUSTANDSVARIABLEN
 * ========================
 * Diese Variablen speichern den aktuellen Zustand des Systems.
 * Sie müssen global sein, da sie zwischen verschiedenen Funktionsaufrufen
 * und über die Zeit hinweg ihren Wert behalten müssen.
 */
int mic_baseline = 0;            // Grundpegel des Mikrofons, wird in setup() kalibriert
                                // Wird von allen Amplitudenmessungen abgezogen, um den "echten" Ausschlag zu messen
unsigned long mic_lastClapTime = 0;  // Zeitpunkt des letzten Klatschens
int mic_clapCount = 0;          // Zählt Klatscher für Doppelklatscher-Erkennung
unsigned long mic_firstClapTime = 0;    // Zeitpunkt des ersten Klatschens (für Doppelklatscher)
bool mic_isLocked = false;      // Sperrstatus nach Doppelklatscher
unsigned long mic_lockUntil = 0;        // Zeitpunkt bis zu dem die Sperre aktiv ist
unsigned long mic_lastSampleTime = 0;  // Zeitpunkt der letzten Messung (für Sample-Intervall)
unsigned long mic_lastCalibrationTime = 0; // Zeitpunkt der letzten Kalibrierung
bool mic_isPotentialClap = false;  // Flag für laufende Klatsch-Erkennung
int mic_maxAmplitude = 0;       // Maximale Amplitude während des aktuellen Events
unsigned long mic_eventStartTime = 0; // Startzeit des aktuellen Events

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
  /*
   * BASELINE-KALIBRIERUNG
   * ====================
   * Ermittelt den Grundpegel des Mikrofons durch Mittelwertbildung.
   * 
   * WICHTIG:
   * - Während der Kalibrierung sollte es möglichst leise sein
   * - Der Wert liegt typischerweise in der Mitte des ADC-Bereichs
   * - Bei 12-Bit etwa um 2048 (4096/2)
   * 
   * MECHANIK:
   * 1. 100 Samples über 1 Sekunde verteilt
   * 2. Durchschnitt = Baseline
   * 3. Große Änderungen werden in der Konsole protokolliert
   */
  
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
  /*
   * SPERRSTATUS-PRÜFUNG
   * ==================
   * Prüft ob eine aktive Sperre abgelaufen ist.
   * Sperren werden nach Doppelklatschern aktiviert.
   * 
   * MECHANIK:
   * - Wenn Sperre aktiv und Sperrzeit abgelaufen:
   *   → Sperre aufheben
   *   → Klatschzähler zurücksetzen
   */
  
  unsigned long currentTime = millis();
  if (mic_isLocked && currentTime >= mic_lockUntil) {
    mic_isLocked = false;
    mic_clapCount = 0;
  }
}

void mic_processClap(float intensity, unsigned long currentTime) {
  /*
   * KLATSCH-VERARBEITUNG
   * ===================
   * Verarbeitet einen erkannten Klatscher und prüft auf Doppelklatscher.
   * 
   * PARAMETER:
   * - intensity: Normalisierte Intensität (0.0-1.0)
   * - currentTime: Aktueller Zeitstempel
   * 
   * MECHANIK:
   * 1. Klatschzähler erhöhen
   * 2. Wenn erster Klatscher:
   *    → Zeit speichern
   * 3. Wenn zweiter Klatscher innerhalb 1 Sekunde:
   *    → Doppelklatscher erkannt
   *    → 5-Sekunden-Sperre aktivieren
   * 4. Wenn zweiter Klatscher nach mehr als 1 Sekunde:
   *    → Als neuer erster Klatscher behandeln
   */
  
  mic_clapCount++;
  if (mic_clapCount == 1) {
    mic_firstClapTime = currentTime;
  }
  else if (mic_clapCount == 2 && (currentTime - mic_firstClapTime) < 1000) {
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
  /*
   * AUDIO-SIGNALVERARBEITUNG
   * ======================
   * Hauptfunktion für die Klatsch-Erkennung.
   * Verarbeitet das Mikrofonsignal und erkennt Klatscher.
   * 
   * MECHANIK:
   * 1. Signal einlesen und Amplitude berechnen
   *    - Rohdaten vom ADC
   *    - Baseline abziehen
   *    - Absoluten Wert nehmen
   * 
   * 2. Klatsch-Erkennung:
   *    a) Wenn kein Event läuft und Signal > NOISE_FLOOR:
   *       → Neues Event starten
   *       → Amplitude speichern
   * 
   *    b) Wenn Event läuft:
   *       → Maximale Amplitude aktualisieren
   *       → Prüfen ob Event beendet (Signal < NOISE_FLOOR oder Timeout)
   * 
   *    c) Bei Event-Ende:
   *       → Normalisierte Intensität berechnen
   *       → Alle Kriterien prüfen
   *       → Ggf. Klatscher verarbeiten
   */
  
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
  /*
   * HAUPTSCHLEIFE
   * ============
   * Koordiniert die zeitgesteuerten Aktionen:
   * 1. Automatische Rekalibrierung (wenn aktiviert)
   * 2. Audio-Signalverarbeitung im definierten Intervall
   * 
   * TIMING:
   * - Rekalibrierung alle 5 Minuten
   * - Audio-Sampling alle 1ms
   */
  
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