# Heimweg-Lichtsystem

Ein vernetztes LED-Beleuchtungssystem basierend auf ESP32-Mikrocontrollern. Das System ermöglicht eine intelligente, bewegungsgesteuerte Beleuchtung mit zentraler Verwaltung und Konfiguration.

## Funktionsweise

### Betriebsmodi & Rollenvergabe
- Automatische Rollenverteilung beim Boot:
  1. DIP-Schalter werden ausgelesen
  2. WLAN-Scan nach existierendem Master
  3. Rollenzuweisung nach folgender Logik:
     - Standalone (DIP 1 = ON): Arbeitet komplett autark und ohne WLAN
     - Master (DIP 2 = ON): 
       - Wird automatisch Standalone wenn bereits Master existiert
         **ODER**
       - Wird Master wenn kein anderer Master gefunden:
         - Öffnet Access Point "LAMPEN_NETZWERK"
         - Feste IP: 192.168.4.1
         - Verwaltet bis zu 25 Slaves:
           - Speichert Device-ID und IP
           - Überwacht Online-Status (Timeout nach 29s)
           - Speichert aktuelle Konfiguration (Modus, Farbe, Helligkeit)
           - Bietet Webinterface zur Steuerung
         - UDP-Kommunikation (Port 4210):
           - Empfängt Status-Updates der Slaves
           - Sendet Befehle an Slaves (Reboot, Modus, Farbe)
           - Bestätigt Empfang durch Fast-Blink
         
     - Slave (DIP 1 & DIP 2 = OFF):
       - Sucht und verbindet sich mit "LAMPEN_NETZWERK"
       - Erhält IP vom Master (DHCP)
       - UDP-Kommunikation (Port 4210):
           - Sendet alle 15s Status-Updates:
             - Device-ID
             - Aktueller Modus (dynamisch/statisch)
             - Aktuelle Farbe
             - Sensor-Modus
           - Empfängt und verarbeitet Master-Befehle
           - Bestätigt Empfang durch Fast-Blink
       - Automatischer Reconnect bei Verbindungsverlust

- Status-LED Signalisierung:
  - Rot: Fehler/Reset
  - Grün: Master (blinkt wenn keine Slaves)
  - Blau: Slave (blinkt wenn keine Verbindung)
  - Weiß: Standalone (blinkt wenn erzwungen)
  - Schnelles Blinken: UDP Austausch

### Push-Button Funktionen
- Visuelles Feedback während des Drückens:
  - Status-LED leuchtet durchgehend rot
- Langes Drücken (< 1 Sekunde) [TODO]:
  - Misst Abstand zum Boden
  - Setzt Bewegungserkennungs-Schwellwert (Distanz - 50cm)
  - Fast-Blink Bestätigung nach Messung
- Sehr langes Drücken (5 Sekunden):
  - System-Reboot
  - Fast-Blink Bestätigung vor Neustart [TODO]

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

## Hardware-Infos
Vibration motor

Parameters：
Rated Voltage: 5.0V DC
Working Voltage: 3.0~5.3V DC
Rated Rotate Speed: Min. 9000RPM
Rated Current: Max. 60mAh
Starting Current: Max. 90mAh
Starting Voltage: DC 3.7V
Insulation Resistance: 10Mohm
Compatibility: Compatible with Arduino / Arduino Mega
Size: 30 x 21 x 6mm/1.8 x 0.82 x 0.23in

Features:
1. Made of highquality mobile phone vibration motor, the vibration effect is obvious.
2. The mos amplification driver can be controlled directly through the digital port of the Arduino.
3. The vibration intensity of the motor can be controlled by PWM.
4. This module can easily convert electrical signals to mechanical vibration.
5. Suitable for the production of vibrationsensitive interactive products, wearable smart device vibration reminders, etc.
