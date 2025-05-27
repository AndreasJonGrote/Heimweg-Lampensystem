// AR on GND, Gain on 5V
#define MIC_PIN 34  // ADC-Eingang für MAX9814

// Debug
#define DEBUG_INTERVAL 100    // Alle 100ms Debug-Ausgabe
unsigned long lastDebugOutput = 0;

// Konstanten für die Klatscherkennung
const int CLAP_THRESHOLD = 900;     // Erhöht, um Musik besser zu filtern
const int MIN_TIME_BETWEEN_CLAPS = 300; // Erhöht, um Mehrfacherkennungen zu vermeiden
const int NOISE_FLOOR = 150;        
const int MAX_CLAP_DURATION = 50;   
const int MIN_INTENSITY = 20;       // Minimale Intensität für gültige Klatscher

// Variablen
int baseline = 0;            // Wird in setup() kalibriert
unsigned long lastClapTime = 0;  // Zeitpunkt des letzten Klatschens
int maxAmplitude = 0;       // Maximale Amplitude während eines Events
bool isPotentialClap = false;  // Flag für mögliches Klatschen
unsigned long eventStartTime = 0; // Startzeit eines Events

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // 12-bit Auflösung (0-4095)
  
  Serial.println("Kalibriere...");
  
  // Baseline kalibrieren
  long sum = 0;
  for(int i = 0; i < 100; i++) {
    sum += analogRead(MIC_PIN);
    delay(10);
  }
  baseline = sum / 100;
  
  Serial.print("Baseline: ");
  Serial.println(baseline);
  Serial.println("-------------------");
}

void loop() {
  // Wert lesen und normalisieren
  int rawValue = analogRead(MIC_PIN);
  int amplitude = abs(rawValue - baseline);
  unsigned long currentTime = millis();
  
  // Debug-Ausgabe der Rohwerte
  if(millis() - lastDebugOutput > DEBUG_INTERVAL) {
    Serial.print("Raw: ");
    Serial.print(rawValue);
    Serial.print(", Amplitude: ");
    Serial.print(amplitude);
    if(isPotentialClap) {
      Serial.print(" [Event läuft, Max: ");
      Serial.print(maxAmplitude);
      Serial.print("]");
    }
    Serial.println();
    lastDebugOutput = millis();
  }
  
  // Event-basierte Erkennung
  if (!isPotentialClap && amplitude > NOISE_FLOOR) {
    isPotentialClap = true;
    eventStartTime = currentTime;
    maxAmplitude = amplitude;
  }
  else if (isPotentialClap) {
    if (amplitude > maxAmplitude) {
      maxAmplitude = amplitude;
    }
    
    // Event ist beendet wenn Amplitude unter NOISE_FLOOR fällt
    // oder MAX_CLAP_DURATION überschritten ist
    if (amplitude < NOISE_FLOOR || (currentTime - eventStartTime) > MAX_CLAP_DURATION) {
      unsigned long eventDuration = currentTime - eventStartTime;
      
      // Intensität berechnen
      int intensity = map(maxAmplitude, CLAP_THRESHOLD, CLAP_THRESHOLD * 2, 0, 100);
      intensity = constrain(intensity, 0, 100);
      
      // Klatschen erkennen wenn:
      // 1. Amplitude über CLAP_THRESHOLD
      // 2. Dauer unter MAX_CLAP_DURATION
      // 3. Genug Zeit seit letztem Klatschen
      // 4. Intensität über Minimum
      if (maxAmplitude > CLAP_THRESHOLD && 
          eventDuration < MAX_CLAP_DURATION && 
          currentTime - lastClapTime > MIN_TIME_BETWEEN_CLAPS &&
          intensity >= MIN_INTENSITY) {
        
        Serial.println("------------------------");
        Serial.print("KLATSCHEN! Intensität: ");
        Serial.print(intensity);
        Serial.println("%");
        Serial.println("------------------------");
        
        lastClapTime = currentTime;
      }
      
      isPotentialClap = false;
      maxAmplitude = 0;
    }
  }
  
  delay(1);
}