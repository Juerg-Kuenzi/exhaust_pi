// Simuliert Motorläufe und prüft die Alarmlogik.
// Bauen und ausführen:
//   g++ -std=c++14 -I../src test_logic.cpp ../src/exhaust_logic.cpp -o test_logic && ./test_logic

#include <cmath>
#include <cstdio>
#include <functional>
#include <random>

#include "exhaust_logic.h"

struct Result {
  double firstWarn = -1, firstAlarm = -1, tempAtAlarm = 0;
  AlarmCause cause = AlarmCause::None;
};

// Läuft 1 Messwert pro Sekunde, Sensor mit 0,1 °C Auflösung und leichtem Rauschen
Result Run(std::function<double(double)> profile, double durationSec) {
  ExhaustLogic logic;
  std::mt19937 rng(42);
  std::normal_distribution<double> noise(0.0, 0.15);
  Result r;
  for (double t = 0; t <= durationSec; t += 1.0) {
    double v = std::round((profile(t) + noise(rng)) * 10.0) / 10.0;
    auto ev = logic.AddSample(t, v);
    if (ev.warnRaised && r.firstWarn < 0) r.firstWarn = t;
    if (ev.alarmRaised && r.firstAlarm < 0) {
      r.firstAlarm = t;
      r.tempAtAlarm = v;
      r.cause = logic.GetCause();
    }
  }
  return r;
}

int failures = 0;

void Check(const char* name, bool ok, const Result& r) {
  const char* cause = r.cause == AlarmCause::Rate ? "Anstieg" :
                      r.cause == AlarmCause::Limit ? "Grenzwert" : "-";
  std::printf("%-46s %s  Vorwarnung: %5.0f s  Alarm: %5.0f s (%s, %.1f °C)\n", name,
              ok ? "OK  " : "FAIL", r.firstWarn, r.firstAlarm, cause, r.tempAtAlarm);
  if (!ok) ++failures;
}

// Aufwärmen nach Kaltstart: nähert sich der Betriebstemperatur an
double WarmUp(double t, double start, double end, double tau) {
  return end - (end - start) * std::exp(-t / tau);
}

int main() {
  // 1. Normaler Kaltstart 18 -> 42 °C: kein Alarm, keine Vorwarnung
  auto r1 = Run([](double t) { return WarmUp(t, 18, 42, 90); }, 1800);
  Check("Kaltstart, langsames Aufwärmen", r1.firstAlarm < 0 && r1.firstWarn < 0, r1);

  // 2. Schneller Kaltstart (tau 45 s) bis 44 °C: kein Alarm
  auto r2 = Run([](double t) { return WarmUp(t, 18, 44, 45); }, 1800);
  Check("Kaltstart, schnelles Aufwärmen", r2.firstAlarm < 0 && r2.firstWarn < 0, r2);

  // 3. Warmstart (Motor kurz aus): 35 -> 42 °C
  auto r3 = Run([](double t) { return WarmUp(t, 35, 42, 30); }, 600);
  Check("Warmstart", r3.firstAlarm < 0 && r3.firstWarn < 0, r3);

  // 4. Totalausfall Impeller nach 10 min Betrieb: +20 °C/min
  auto r4 = Run([](double t) {
    double base = WarmUp(t, 18, 42, 90);
    return t < 600 ? base : base + (t - 600) * 20.0 / 60.0;
  }, 900);
  Check("Impeller-Totalausfall (+20 °C/min)",
        r4.firstAlarm > 600 && r4.cause == AlarmCause::Rate && r4.tempAtAlarm < 60, r4);

  // 5. Teildefekt: Temperatur steigt langsam (1 °C/min) auf 54 °C
  auto r5 = Run([](double t) {
    double base = WarmUp(t, 18, 42, 90);
    return t < 600 ? base : std::min(54.0, base + (t - 600) / 60.0);
  }, 2400);
  Check("Impeller-Teildefekt (langsam auf 54 °C)", r5.firstWarn > 600 && r5.firstAlarm < 0, r5);

  // 6. Langsamer Anstieg bis über den Alarmwert
  auto r6 = Run([](double t) {
    double base = WarmUp(t, 18, 42, 90);
    return t < 600 ? base : base + (t - 600) * 2.0 / 60.0;
  }, 1800);
  Check("Langsamer Anstieg bis über 60 °C",
        r6.firstWarn > 0 && r6.firstAlarm > r6.firstWarn && r6.cause == AlarmCause::Limit, r6);

  // 7. Totalausfall direkt beim Start (unter Betriebstemperatur) -> spätestens Grenzwert
  auto r7 = Run([](double t) { return 18 + t * 20.0 / 60.0; }, 300);
  Check("Ausfall beim Start (Anstieg ab 18 °C)", r7.firstAlarm > 0, r7);

  // 8. Abklingen nach Alarm: Alarm muss enden
  {
    ExhaustLogic l;
    for (int t = 0; t < 60; ++t) l.AddSample(t, 65);
    for (int t = 60; t < 120; ++t) l.AddSample(t, 30);
    bool ok = l.GetLevel() == Level::Normal;
    std::printf("%-46s %s\n", "Alarm endet nach Abkühlen", ok ? "OK  " : "FAIL");
    if (!ok) ++failures;
  }

  std::printf("\n%s\n", failures ? "FEHLER" : "Alle Szenarien bestanden.");
  return failures ? 1 : 0;
}
