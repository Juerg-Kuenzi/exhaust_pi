// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#pragma once
// Gemeinsame Typen für das Abgastemperatur-Plugin

#include <wx/string.h>

#include "exhaust_logic.h"

// UTF-8-Literale sicher in wxString umwandeln (Umlaute, Gradzeichen)
#define U8(s) wxString::FromUTF8(s)

struct ExhaustSettings {
  LogicParams alarm;             // Vorwarnung, Alarm, Hysterese, Betriebstemp., Anstieg
  wxString xdrName = "EXHAUST";  // Name des XDR-Messwertgebers, leer = erster Temperaturwert
  int timeoutSec = 10;           // Sekunden ohne Daten -> "Datenausfall"
  bool soundEnabled = true;      // akustischer Alarm
  bool alarmOnLoss = true;       // auch bei Datenausfall alarmieren
  wxString soundFile;            // WAV-Datei, leer = OpenCPN-Standardton
};

struct DisplayState {
  bool haveValue = false;
  bool dataLost = false;
  double temp = 0.0;
  LogicParams params;
  Level level = Level::Normal;
  AlarmCause cause = AlarmCause::None;
  bool acked = false;
  bool rateArmed = false;   // Temperatur über Betriebstemperatur, Anstieg wird überwacht
  bool rateValid = false;
  double rate = 0.0;        // °C/min
  bool ackPossible = false;
};
