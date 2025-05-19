// Einbinden der Preferences-Bibliothek, die das dauerhafte Speichern von Werten im Flash-Speicher ermöglicht
#include <Preferences.h>

Preferences globals;

void setup() {
  Serial.begin(115200);
}

// Hauptschleife - hier passiert nichts, da wir nur einmalig die Werte speichern/auslesen
void loop() {
  globals.begin("globals", true);  // LESEN aus dem NVS
  String id = globals.getString("device_id", "undefined");
  globals.end();

  Serial.println("device_id vorhanden: " + id);
  delay(500);
}