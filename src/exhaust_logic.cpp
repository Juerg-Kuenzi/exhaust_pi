// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#include "exhaust_logic.h"

// Steigung per linearer Regression über das Zeitfenster.
// Glättet das Rauschen des Sensors besser als die Differenz zweier Werte.
// Es zählen nur Messwerte oberhalb der minimalen Betriebstemperatur: So fließt
// das Aufwärmen nach dem Motorstart nicht in die Rate ein, auch wenn die
// Betriebstemperatur nur knapp über der Schwelle liegt.
void ExhaustLogic::UpdateRate(double t, double v) {
  if (!m_hist.empty() && t <= m_hist.back().t) m_hist.clear();  // Zeitsprung
  if (v <= m_p.minOpTemp) {  // unter Betriebstemperatur: Verlauf neu beginnen
    m_hist.clear();
    m_rateValid = false;
    return;
  }
  m_hist.push_back({t, v});
  while (!m_hist.empty() && t - m_hist.front().t > kRateWindowSec) m_hist.pop_front();

  m_rateValid = false;
  if (static_cast<int>(m_hist.size()) < kRateMinSamples) return;
  if (m_hist.back().t - m_hist.front().t < kRateMinSpanSec) return;

  const double t0 = m_hist.front().t;
  double n = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (const auto& s : m_hist) {
    const double x = s.t - t0;
    n += 1; sx += x; sy += s.v; sxx += x * x; sxy += x * s.v;
  }
  const double den = n * sxx - sx * sx;
  if (den <= 0) return;
  m_rate = (n * sxy - sx * sy) / den * 60.0;  // °C/s -> °C/min
  m_rateValid = true;
}

ExhaustLogic::Events ExhaustLogic::AddSample(double tSec, double temp) {
  Events ev;
  UpdateRate(tSec, temp);

  const bool limitCond = temp > m_p.limit;
  const bool rateOn = m_p.rateLimit > 0 && m_rateValid && temp > m_p.minOpTemp;
  const bool rateCond = rateOn && m_rate > m_p.rateLimit;

  if (m_level != Level::Alarm) {
    if (limitCond || rateCond) {
      m_level = Level::Alarm;
      m_cause = limitCond ? AlarmCause::Limit : AlarmCause::Rate;
      m_acked = false;
      ev.alarmRaised = true;
      return ev;
    }
  } else {
    // Anstiegsalarm quittiert, jetzt auch Grenzwert überschritten -> erneut alarmieren
    if (limitCond && m_cause == AlarmCause::Rate) {
      m_cause = AlarmCause::Limit;
      m_acked = false;
      ev.alarmRaised = true;
      return ev;
    }
    // Alarm endet erst, wenn Temperatur deutlich unter der Grenze liegt
    // und der Anstieg auf weniger als die Hälfte des Grenzwerts abgeklungen ist.
    const bool belowLimit = temp < m_p.limit - m_p.hysteresis;
    const bool rateCalm = !(rateOn && m_rate > m_p.rateLimit * 0.5);
    if (belowLimit && rateCalm) {
      m_level = temp > m_p.warn - m_p.hysteresis ? Level::Warning : Level::Normal;
      m_cause = AlarmCause::None;
      m_acked = false;
      ev.cleared = true;
    }
    return ev;
  }

  // Vorwarnung
  if (m_level == Level::Normal && temp > m_p.warn) {
    m_level = Level::Warning;
    ev.warnRaised = true;
  } else if (m_level == Level::Warning && temp < m_p.warn - m_p.hysteresis) {
    m_level = Level::Normal;
    ev.cleared = true;
  }
  return ev;
}
