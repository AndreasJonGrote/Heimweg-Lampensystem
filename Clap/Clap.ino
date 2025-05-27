// AR on GND, Gain on 5V
#define MIC_PIN 34  // ADC-Eingang für MAX9814

// Konstanten für die Klatscherkennung
const int mic_CLAP_THRESHOLD = 900;     // Erhöht, um Musik besser zu filtern
const int mic_MIN_TIME_BETWEEN_CLAPS = 300; // Erhöht, um Mehrfacherkennungen zu vermeiden
const int mic_NOISE_FLOOR = 150;        
const int mic_MAX_CLAP_DURATION = 50;   
const float mic_MIN_INTENSITY = 0.20;   // Minimale Intensität (0.0-1.0) für gültige Klatscher
const int mic_DOUBLE_CLAP_LOCKTIME = 5000; // Sperrzeit nach Doppelklatscher in ms
const unsigned long mic_SAMPLE_INTERVAL = 1; // Minimale Zeit zwischen Samples in ms

// Globale Zustandsvariablen
int mic_baseline = 0;            // Wird in setup() kalibriert
unsigned long mic_lastClapTime = 0;  // Zeitpunkt des letzten Klatschens
int mic_clapCount = 0;          // Zählt Klatscher
unsigned long mic_firstClapTime = 0;    // Zeitpunkt des ersten Klatschens
bool mic_isLocked = false;      // Sperrstatus
unsigned long mic_lockUntil = 0;        // Sperrzeit bis zu diesem Zeitpunkt
unsigned long mic_lastSampleTime = 0;  // Zeitpunkt der letzten Messung

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
}

void mic_calibrateBaseline() {
  long sum = 0;
  for(int i = 0; i < 100; i++) {
    sum += analogRead(MIC_PIN);
    delay(10);
  }
  mic_baseline = sum / 100;
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
    // Beispiel:
    // void processIntensity(float value) {
    //   // value ist normalisiert zwischen 0.0 und 1.0
    //   // z.B. LED-Helligkeit oder andere Aktionen basierend auf der Intensität
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
  static bool mic_isPotentialClap = false;  // Lokale statische Variable
  static int mic_maxAmplitude = 0;          // Lokale statische Variable
  static unsigned long mic_eventStartTime = 0; // Lokale statische Variable
  
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
  
  // Prüfe ob es Zeit für ein neues Sample ist
  if (currentTime - mic_lastSampleTime >= mic_SAMPLE_INTERVAL) {
    mic_lastSampleTime = currentTime;
    mic_checkLockStatus();
    mic_processAudio();
  }
}