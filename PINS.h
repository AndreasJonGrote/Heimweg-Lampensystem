// ==== LED RING DEFINITIONEN WS2812B ====
const int LED_RING_PIN = 4;
const int LED_RING_COUNT = 16;

// ==== ULTRASCHALL HC-SR04 ====
const int SONICSENSOR_TRIG_PIN = 21; // War auf 5, jetzt 21
const int SONICSENSOR_ECHO_PIN = 18;

// ==== DIP-SCHALTER (Konfigurations-GPIOs) ====
const int DIP_STANDALONE_PIN    = 13; // DIP 1 – WLAN aus (lokal)
const int DIP_ACCESS_POINT_PIN  = 25; // DIP 2 – Master: Access Point starten
const int DIP_DYNAMIC_MODE_PIN  = 33; // DIP 3 – Modus: dynamisch vs. statisch (z. B. 80 %)
const int DIP_DYNAMIC_COLOR_PIN = 32; // DIP 4 – Farbe: dynamisch oder fix weiß

// ==== RGB STATUSLED (KY-016, gemeinsame Kathode) ====
const int STATUS_LED_R_PIN = 27;
const int STATUS_LED_G_PIN = 14;
const int STATUS_LED_B_PIN = 26;

// ==== BUTTONS ====
const int BUTTON_PUSH_PIN = 19; // Push Button – Reboot

// ==== TBA: Vibrationsmotor ====
const int VIBRATION_MOTOR_PIN = 23;

// ==== TBA: Mikrofon (MAX9814 AGC) ====
const int MIC_PIN = 34;

