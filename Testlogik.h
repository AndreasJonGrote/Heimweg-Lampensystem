// ==== RGB STATUSLED (KY-016, gemeinsame Kathode) ====
const int LED_R = 27;
const int LED_G = 12;
const int LED_B = 26;

// ==== Farben als ENUM ====
enum LedColor {
  COLOR_OFF,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_WHITE
};

// ==== Globale LED-Zustände ====
LedColor STATUS_LED_COLOR = COLOR_OFF;
bool STATUS_LED_BLINK = false;

// ==== Blinksteuerung ====
unsigned long last_blink_time = 0;
bool led_visible = true;

void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
}

void loop() {
  // Beispiel: nach 8 Sekunden auf blinkendes Blau umstellen
  static unsigned long timer = millis();
  if (millis() - timer > 8000) {
    STATUS_LED_COLOR = COLOR_BLUE;
    STATUS_LED_BLINK = true;
  }

  if (STATUS_LED_COLOR == COLOR_OFF) {
    setColor(false, false, false);  // direkt aus
  } else {
    set_status_led(STATUS_LED_COLOR, STATUS_LED_BLINK);
  }

  // Platz für weitere Logik ...
}

// ==== LED-Funktion mit Blinkverhalten ====
void set_status_led(LedColor color, bool should_blink) {
  if (should_blink) {
    if (millis() - last_blink_time >= 1000) {
      led_visible = !led_visible;
      last_blink_time = millis();
    }
  } else {
    led_visible = true;
  }

  if (!led_visible) {
    setColor(false, false, false);
    return;
  }

  switch (color) {
    case COLOR_RED:   setColor(true, false, false); break;
    case COLOR_GREEN: setColor(false, true, false); break;
    case COLOR_BLUE:  setColor(false, false, true); break;
    case COLOR_WHITE: setColor(true, true, true);   break;
    default:          setColor(false, false, false); break;
  }
}

// ==== LED ansteuern ====
void setColor(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? HIGH : LOW);
  digitalWrite(LED_G, g ? HIGH : LOW);
  digitalWrite(LED_B, b ? HIGH : LOW);
}