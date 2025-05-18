// Einbinden der Preferences-Bibliothek, die das dauerhafte Speichern von Werten im Flash-Speicher ermöglicht
#include <Preferences.h>

// Definition der Standardwerte für Geräte-ID und Version
const char* device_id       = "A001";
const char* device_version  = "0.0.1a";

// Erstellen eines Preferences-Objekts für den Zugriff auf den Flash-Speicher
Preferences globals;

void setup() {
  // Initialisierung der seriellen Kommunikation mit 115200 Baud
  Serial.begin(115200);
  // Kurze Verzögerung, um sicherzustellen, dass die serielle Verbindung bereit ist
  delay(500);

  // Öffnen des Preferences-Namespace "globals" im Lese-/Schreibmodus (false = read/write)
  globals.begin("globals", false);

  // Prüfen, ob bereits eine Geräte-ID gespeichert ist
  if (!globals.isKey("device_id")) {
    // Wenn keine ID vorhanden ist, speichere die Standard-ID
    globals.putString("device_id", device_id);
    Serial.println("device_id gespeichert: " + String(device_id));
  } else {
    // Wenn eine ID vorhanden ist, gebe sie aus
    Serial.println("device_id vorhanden: " + globals.getString("device_id"));
  }

  // Prüfen, ob bereits eine Geräteversion gespeichert ist
  if (!globals.isKey("device_version")) {
    // Wenn keine Version vorhanden ist, speichere die Standardversion
    globals.putString("device_version", device_version);
    Serial.println("device_version gespeichert: " + String(device_version));
  } else {
    // Wenn eine Version vorhanden ist, gebe sie aus
    Serial.println("device_version vorhanden: " + globals.getString("device_version"));
  }

  // Schließen des Preferences-Namespace, um Ressourcen freizugeben
  globals.end();
}

// Hauptschleife - hier passiert nichts, da wir nur einmalig die Werte speichern/auslesen
void loop() { }