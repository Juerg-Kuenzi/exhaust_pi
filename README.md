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

## Fertiges Paket von GitHub (ohne eigene Installation)

Bei jedem Push baut GitHub das Plugin automatisch für Windows (32 und 64 Bit):

1. Im Repository auf **Actions** klicken und den obersten grünen Lauf öffnen.
2. Unten unter **Artifacts** `exhaust_pi-windows-Win32` oder `exhaust_pi-windows-x64` herunterladen und entpacken. Darin liegt eine `.tar.gz`-Datei.
3. In OpenCPN: Optionen → Plugins → **Plugin importieren** → die `.tar.gz`-Datei wählen.
4. „ExhaustTemp“ in der Liste aktivieren.

Welche Variante passt, hängt von der OpenCPN-Installation ab (Hilfe → Info). Wird die falsche gewählt, lehnt OpenCPN den Import ab.

## Selbst bauen unter Windows

Der Build-Rahmen (CMake-Skripte, Windows-Skripte, Paketierung) stammt aus der Plugin-Vorlage [shipdriver_pi](https://github.com/Rasbats/shipdriver_pi) und ist fest in dieses Repository kopiert. Die OpenCPN-Schnittstelle kommt als Git-Submodul `opencpn-libs`.

**Einmalig installieren** (PowerShell als Administrator):

1. Chocolatey: Befehl von <https://chocolatey.org/install> ausführen.
2. Compiler und Git:
   ```
   choco install -y git visualstudio2022buildtools visualstudio2022-workload-vctools
   ```
3. Repository holen (normale Eingabeaufforderung, `cmd`):
   ```
   git clone --recurse-submodules https://github.com/Juerg-Kuenzi/exhaust_pi.git
   cd exhaust_pi
   ```
4. Restliche Werkzeuge (CMake, Python, gettext, 7-Zip, wxWidgets) – in einer **Eingabeaufforderung als Administrator** im Ordner `exhaust_pi`:
   ```
   buildwin\win_deps.bat
   ```

**Bauen** (normale Eingabeaufforderung im Ordner `exhaust_pi`):

```
ci\appveyor.bat
```

Für 64 Bit vorher `set PLATFORM=x64` eingeben. Das fertige Paket liegt danach als `.tar.gz` im Ordner `build` und wird wie oben beschrieben importiert. Die Hinweise auf fehlende Cloudsmith- oder Git-Schlüssel am Ende sind normal: Sie betreffen nur das Hochladen in den offiziellen Plugin-Katalog.

Wurde das Repository ohne `--recurse-submodules` geklont, holt `git submodule update --init` das Submodul nach.

## Bauen unter Linux

```bash
git clone --recurse-submodules https://github.com/Juerg-Kuenzi/exhaust_pi.git
cd exhaust_pi && mkdir build && cd build
cmake .. && make tarball
```

Benötigt CMake, einen C++-Compiler, gettext und die wxWidgets-Entwicklungspakete (Version 3.2).

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
- Das Fenster zeigt nur die Temperatur und die Taste „Quittieren“. Die Schrift entspricht den Messwerten im Dashboard (dort geänderte Schrift wird übernommen). Zustand, Anstiegsrate und Schwellen erscheinen als Tooltip, wenn die Maus über der Anzeige steht.
- **Normal:** Standardfarben.
- **Vorwarnung:** gelb, einmaliger Ton.
- **Alarm:** rot, Ton alle 3 Sekunden; die Ursache (Alarmwert oder schneller Anstieg) steht im Tooltip. „Quittieren“ stoppt den Ton, die Anzeige bleibt orange, bis der Alarm beendet ist. Wird ein quittierter Anstiegsalarm danach vom Alarmwert überholt, ertönt der Alarm erneut.
- **Datenausfall:** kommen länger als der eingestellte Timeout keine Werte, wird die Anzeige grau und zeigt „---“, auf Wunsch mit Ton alle 10 Sekunden.

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

## Lizenz

Copyright (C) 2026 Juerg Kuenzi

Dieses Programm ist freie Software. Die eigenen Quelltexte (`src/`, `tests/`, `tools/`) dürfen unter den Bedingungen der GNU General Public License, Version 2 oder (nach Wahl) jeder späteren Version, weitergegeben und/oder verändert werden ([LICENSE](LICENSE)).

Der aus shipdriver_pi übernommene Build-Rahmen (`cmake/`, `ci/`, `buildwin/`, `CMakeLists.txt`) steht laut den Dateiköpfen unter der GPL Version 3 oder neuer. Das fertige Plugin-Paket wird deshalb als Ganzes unter der GPL Version 3 weitergegeben.

Das Plugin ersetzt keine vom Motorhersteller vorgesehene Überwachung und keine regelmäßige Sichtkontrolle des Kühlwasseraustritts. Es wird ohne jede Gewährleistung bereitgestellt.
