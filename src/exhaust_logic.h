// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#pragma once
// Alarmlogik für die Abgastemperatur – bewusst ohne wxWidgets,
// damit sie unabhängig von OpenCPN getestet werden kann.
//
// Drei Überwachungen:
//  1. Vorwarnung:   Temperatur > Vorwarnwert            (z. B. Teildefekt Impeller)
//  2. Alarm:        Temperatur > Alarmwert
//  3. Anstiegsalarm: Anstieg > x °C/min, aber nur oberhalb der minimalen
//                   Betriebstemperatur – so löst das Aufwärmen nach dem
//                   Motorstart keinen Alarm aus.

#include <deque>

enum class Level { Normal, Warning, Alarm };
enum class AlarmCause { None, Limit, Rate };

struct LogicParams {
  double warn = 50.0;        // Vorwarnung ab (°C)
  double limit = 60.0;       // Alarm ab (°C)
  double hysteresis = 5.0;   // Rückfall erst unter Wert - Hysterese
  double minOpTemp = 40.0;   // Anstiegsüberwachung erst oberhalb (°C)
  double rateLimit = 5.0;    // Anstiegsalarm ab (°C/min), 0 = aus
};

class ExhaustLogic {
public:
  // Auswertungsfenster für die Anstiegsrate
  static constexpr double kRateWindowSec = 30.0;  // Messwerte der letzten 30 s
  static constexpr double kRateMinSpanSec = 15.0; // mindestens 15 s Daten nötig
  static constexpr int kRateMinSamples = 5;

  struct Events {
    bool warnRaised = false;   // Vorwarnung neu
    bool alarmRaised = false;  // Alarm neu (oder Ursache verschärft)
    bool cleared = false;      // Alarm/Vorwarnung beendet
  };

  void SetParams(const LogicParams& p) { m_p = p; }
  const LogicParams& Params() const { return m_p; }

  // Neuer Messwert: Zeit in Sekunden (monoton), Temperatur in °C
  Events AddSample(double tSec, double temp);

  // Verlauf verwerfen (z. B. nach Datenausfall), Alarmzustand bleibt
  void ResetHistory() { m_hist.clear(); m_rateValid = false; }

  void Acknowledge() { if (m_level == Level::Alarm) m_acked = true; }

  Level GetLevel() const { return m_level; }
  bool IsAcked() const { return m_acked; }
  AlarmCause GetCause() const { return m_cause; }
  bool RateValid() const { return m_rateValid; }
  double Rate() const { return m_rate; }  // °C/min
  bool RateArmed(double temp) const {
    return m_p.rateLimit > 0 && temp > m_p.minOpTemp;
  }

private:
  struct Sample { double t, v; };
  void UpdateRate(double t, double v);

  LogicParams m_p;
  std::deque<Sample> m_hist;
  bool m_rateValid = false;
  double m_rate = 0.0;

  Level m_level = Level::Normal;
  AlarmCause m_cause = AlarmCause::None;
  bool m_acked = false;
};
