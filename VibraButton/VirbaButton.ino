const int motorPin = 18;     // Vibrationsmotor
const int buttonPin = 19;    // Taster
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);
  pinMode(motorPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // interner Pullup aktiviert
  Serial.println("Bereit – drück den Knopf!");
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {  // Button gedrückt (LOW wegen Pullup)
    if (!buttonPressed) {  // Flanke erkennen
      buttonPressed = true;
      Serial.println("Button gedrückt – Muster startet!");
      vibrieren();
    }
  } else {
    buttonPressed = false;
  }
}

// Funktion für Vibrationsmuster
void vibrieren() {
  // Beispiel-Muster: {An-Zeit, Aus-Zeit, An-Zeit}
  int muster[] = {200, 100, 300};  // in Millisekunden

  for (int i = 0; i < sizeof(muster) / sizeof(muster[0]); i++) {
    if (i % 2 == 0) {
      digitalWrite(motorPin, HIGH);  // Motor an
    } else {
      digitalWrite(motorPin, LOW);   // Motor aus
    }
    delay(muster[i]);
  }

  digitalWrite(motorPin, LOW);  // sicherstellen, dass Motor aus ist
}