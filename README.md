# Heimweg-Lichtsystem

Ein vernetztes LED-Beleuchtungssystem basierend auf ESP32-Mikrocontrollern. Das System ermöglicht eine intelligente, bewegungsgesteuerte Beleuchtung mit zentraler Verwaltung und Konfiguration.

## Funktionsweise

### Netzwerk-Architektur
- Ein ESP32 fungiert als Master (Access Point)
- Weitere ESPs verbinden sich als Slaves
- Kommunikation über UDP (Port 4210)
- Automatische Slave-Erkennung und -Verwaltung
- Slaves senden alle 15 Sekunden Status-Updates

### Bewegungs- und Geräuscherkennung
- Ultraschall (HC-SR04):
  - Bewegungserkennung im Bereich 10-150cm
  - Auto-Leveling via Push-Button (Distanz - 50cm als Schwellwert)
  - Gleitende Mittelwertbildung über 5 Messungen
- Mikrofon (TODO):
  - MAX9814 AGC geplant
  - Geräuscherkennung für zusätzliche Aktivierung
  - Gleitende Mittelwertbildung über 15 Sekunden

### LED-Steuerung
- WS2812B LED-Ring (16 LEDs):
  - Sanftes Ein-/Ausblenden (Sinus-Easing)
  - Konfigurierbare Helligkeit
  - Dynamischer oder statischer Modus
  - Optionaler Farbwechsel-Modus
- Status-LED (RGB):
  - Rot: Fehler/Reset
  - Grün: Master-Modus
  - Blau: Slave-Modus
  - Weiß: Standalone
  - Blinken: Keine Verbindung/Keine Slaves
  - Schnelles Blinken: UDP Austausch

### Konfiguration
- DIP-Schalter:
  1. Standalone-Modus (WLAN aus)
  2. Master/Slave-Modus
  3. Dynamisch/Statisch
  4. Dynamische/Fixe Farbe
- Persistente Einstellungen:
  - Eindeutige Device-ID
  - Security-Token (TODO)
  - Betriebsmodus

### Webinterface
- Erreichbar über Master-IP
- Funktionen:
  - Übersicht aller verbundenen Geräte
  - Remote-Reboot einzelner Geräte
  - Moduswechsel
  - Farbsteuerung
- Auto-Refresh alle 30 Sekunden

## Hardware-Komponenten (pro Einheit)

### Mikrocontroller & Sensoren
- ESP32 DevKit V1 Board
- Ultraschallsensor HC-SR04
- LED-Ring WS2812B (16 LEDs)
- RGB Status-LED (KY-016, gemeinsame Kathode)
- 4x DIP-Schalter für Konfiguration
- Reset-Taster

### Geplante Erweiterungen
- Vibrationsmotor (PIN vorbereitet)
- Mikrofon MAX9814 AGC (PIN vorbereitet)

### Stromanforderungen (TODO)
- ESP32: ~180mA (mit WiFi)
- HC-SR04: ~15-20mA
- LED-Ring: bis zu 960mA (bei 100% weiß)
- Empfohlene Stromversorgung: 5V/2A Netzteil
- Separate Stromversorgung für LED-Ring zu prüfen

## Pin-Belegung

### Sensoren & Aktoren
- LED-Ring: PIN 4
- Ultraschall TRIG: PIN 5
- Ultraschall ECHO: PIN 18
- Status-LED (R/G/B): PIN 27/14/26
- Reset-Taster: PIN 19

### Konfiguration (DIP-Schalter)
- DIP 1 (PIN 13): Standalone-Modus
- DIP 2 (PIN 25): Access Point Modus
- DIP 3 (PIN 33): Dynamischer/Statischer Modus
- DIP 4 (PIN 32): Dynamische/Fixe Farbe

## Installation & Setup

### Ersteinrichtung
1. Seriennummer programmieren
2. Security-Token generieren (TODO)
3. DIP-Schalter entsprechend der Rolle konfigurieren:
   - Master: DIP 2 = ON
   - Slave: Alle DIP = OFF
   - Standalone: DIP 1 = ON

## Offene Punkte (TODO)

### Hardware
- [ ] Finale Stromversorgung konzipieren
- [ ] Gehäuse-Design erstellen
- [ ] Kabelführung optimieren
- [ ] Schutzschaltungen implementieren
- [ ] Mikrofon-Integration

### Software
- [ ] Token-System implementieren
- [ ] Mikrofon-Erkennung implementieren
- [ ] Erweiterte Weboberfläche
- [ ] Auto-Leveling für Ultraschall

### Dokumentation
- [ ] Gehäuse-Zeichnungen
- [ ] Verkabelungsplan
- [ ] Konfigurationsanleitung
- [ ] Wartungshandbuch

## Technische Spezifikationen

### Netzwerk
- WLAN: 2.4 GHz
- Protokoll: UDP (Port 4210)
- Web-Interface: Port 80
- Max. Slaves: 25 Geräte
- Update-Intervall: 15 Sekunden

### Sensoren
- Ultraschall-Reichweite: Auto-Leveling (Distanz - 50cm)
- LED-Helligkeit
- Updaterate: 20ms (LED-Animation), 100ms (Sensor)

### Speicher
- Flash: ~440KB verwendet
- RAM: ~21KB verwendet
- NVS: Device-ID, Token, Konfiguration

## Sicherheitshinweise

- Ausreichende Stromversorgung sicherstellen
- Überhitzungsschutz beachten
- Wasserschutz im Gehäuse vorsehen

