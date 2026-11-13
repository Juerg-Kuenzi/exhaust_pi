# ExhaustTemp – Abgastemperatur-Plugin für OpenCPN

Zeigt die Abgastemperatur eines nassen Auspuffs groß als Zahl an und überwacht sie dreifach: Vorwarnstufe (Teildefekt des Impellers), Alarmwert und Anstiegsgeschwindigkeit (Totalausfall des Impellers). Optional wird auch ein Datenausfall gemeldet.

## Datenformat

Das Plugin wertet NMEA-0183-**XDR**-Sätze aus, die über eine normale OpenCPN-Verbindung hereinkommen:

```
$YDXDR,P,101320,P,Baro,C,34.2,C,EXHAUST*31
       └──── Luftdruck ────┘ └── Abgastemperatur ─┘

Temperaturgruppe: C = Typ Temperatur, 34.2 = Wert,
C = Einheit (C, F oder K, wird nach °C umgerechnet), EXHAUST = Name (einstellbar)
```

Die Prüfsumme wird kontrolliert, falls vorhanden. Ein Satz mit mehreren Messwertgebern ist erlaubt; das Plugin nimmt den Eintrag mit dem eingestellten Namen (oder den ersten Temperaturwert, wenn kein Name eingestellt ist).

Wichtig: OpenCPN leitet über Netzwerkverbindungen nur NMEA-Sätze (beginnend mit `$` oder `!`) an Plugins weiter. Sendet die Quelle z. B. nur eine nackte Zahl, muss das Format angepasst werden.

## Verbindung in OpenCPN

Optionen → Verbindungen → Verbindung hinzufügen:

- Typ: Netzwerk
- Protokoll: UDP
- Adresse: `0.0.0.0`
- Port: `1456`
- Richtung: Empfangen
- Keine Eingangsfilter, die XDR-Sätze blockieren

## Bauen (Linux)

Benötigt: CMake, C++-Compiler, wxWidgets-Entwicklungspakete (dieselbe Version wie OpenCPN, meist 3.2) und `ocpn_plugin.h` aus den OpenCPN-Quellen.

```bash
git clone --depth 1 https://github.com/OpenCPN/OpenCPN.git
mkdir build && cd build
cmake .. -DOCPN_API_DIR=../OpenCPN/include
make
mkdir -p ~/.local/lib/opencpn
cp libexhaust_pi.so ~/.local/lib/opencpn/
```

Danach OpenCPN neu starten und unter Optionen → Plugins „ExhaustTemp“ aktivieren.

Für Windows, macOS oder ein Installationspaket für den Plugin-Katalog empfiehlt sich die offizielle Plugin-Vorlage **shipdriver_pi** (github.com/Rasbats/shipdriver_pi): einfach die Dateien aus `src/` übernehmen und die Klassennamen in der Vorlage anpassen. Die Vorlage kümmert sich um Linking gegen OpenCPN, Packaging und Signierung.

## Überwachung

| Einstellung | Standard | Wirkung |
|---|---|---|
| Minimale Betriebstemperatur | 40 °C | Erst darüber wird der Anstieg überwacht. Das Aufwärmen nach dem Motorstart löst so keinen Alarm aus. |
| Anstiegsalarm | 5 °C/min | Alarm, wenn die Temperatur oberhalb der Betriebstemperatur schneller steigt (0 = aus). |
| Vorwarnung | 50 °C | Gelbe Anzeige und einmaliger Ton, z. B. bei reduziertem Kühlwasserfluss. |
| Alarm | 60 °C | Rote Anzeige, Ton alle 3 s bis zur Quittierung. |
| Hysterese | 5 °C | Vorwarnung/Alarm enden erst um diesen Betrag unter dem Wert. |

**Anstiegsberechnung:** lineare Regression über die Messwerte der letzten 30 Sekunden (mindestens 15 s Daten). Es zählen nur Messwerte oberhalb der minimalen Betriebstemperatur. Fällt die Temperatur darunter, beginnt die Berechnung neu. Damit stört das Aufwärmen auch dann nicht, wenn die Betriebstemperatur nur knapp über der Schwelle liegt.

**Einstellen der Betriebstemperatur:** etwas unter der Temperatur wählen, die der Motor im normalen Betrieb sicher erreicht. Liegt sie über der normalen Betriebstemperatur, ist der Anstiegsalarm im Normalbetrieb nicht aktiv und greift erst, wenn die Temperatur die Schwelle überschreitet. Das funktioniert auch, erkennt einen Ausfall aber etwas später.

Ein schneller Anstieg unterhalb der Betriebstemperatur (z. B. Impeller schon beim Start defekt) wird erkannt, sobald die Temperatur die Schwelle überschreitet.

## Bedienung

- Toolbar-Symbol (Thermometer) blendet das Fenster ein und aus. Bei Vorwarnung oder Alarm wird es automatisch eingeblendet.
- Unter der Temperatur steht die aktuelle Anstiegsrate in °C/min, bzw. der Hinweis, dass die Temperatur noch unter der Betriebstemperatur liegt.
- **Normal:** Standardfarben, Status „OK“.
- **Vorwarnung:** gelb, einmaliger Ton.
- **Alarm:** rot, Ton alle 3 Sekunden, mit Angabe der Ursache (Alarmwert oder schneller Anstieg). „Alarm quittieren“ stoppt den Ton, die Anzeige bleibt orange, bis der Alarm beendet ist. Wird ein quittierter Anstiegsalarm danach vom Alarmwert überholt, ertönt der Alarm erneut.
- **Datenausfall:** kommen länger als der eingestellte Timeout keine Werte, wird die Anzeige grau, auf Wunsch mit Ton alle 10 Sekunden.

Einstellungen: Optionen → Plugins → ExhaustTemp → Einstellungen. Gespeichert wird in der OpenCPN-Konfiguration unter `[PlugIns/ExhaustTemp]`. Alarmereignisse erscheinen zusätzlich im OpenCPN-Log.

## Testen ohne Motor

Alarmlogik mit simulierten Motorläufen prüfen (kein OpenCPN nötig):

```bash
g++ -std=c++14 -Isrc tests/test_logic.cpp src/exhaust_logic.cpp -o test_logic && ./test_logic
```

Plugin in OpenCPN mit simulierten Daten testen:

```bash
python3 tools/test_sender.py --scenario impeller --fail-after 180   # Anstiegsalarm
python3 tools/test_sender.py --scenario teildefekt --fail-after 180 # nur Vorwarnung
python3 tools/test_sender.py --scenario sinus --min 30 --max 70     # alles durchlaufen
```

Nach Strg+C meldet das Plugin nach dem Timeout den Datenausfall.
