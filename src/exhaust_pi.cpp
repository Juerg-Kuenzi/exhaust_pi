// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

// OpenCPN-Plugin: Abgastemperatur anzeigen und überwachen
//
// Erwartet NMEA-0183-XDR-Sätze, z. B.
//   $IIXDR,C,412.5,C,EXHAUST*hh
// (Typ C = Temperatur, Wert, Einheit C/F/K, Name des Messwertgebers)
// Die Daten kommen über eine normale OpenCPN-Verbindung (z. B. UDP Port 1456).

#include "exhaust_pi.h"

#include <wx/dcmemory.h>
#include <wx/display.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

#include "ExhaustWindow.h"
#include "PrefsDialog.h"

// --- Einstiegspunkte für OpenCPN -------------------------------------------

extern "C" DECL_EXP opencpn_plugin* create_pi(void* ppimgr) {
  return new exhaust_pi(ppimgr);
}

extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) { delete p; }

// --- Hilfsfunktionen --------------------------------------------------------

namespace {

const char* kConfigPath = "/PlugIns/ExhaustTemp";

// Einfaches Thermometer-Symbol, zur Laufzeit gezeichnet
wxBitmap MakeIcon(int s) {
  wxBitmap bmp(s, s);
  {
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const int cx = s / 2;
    dc.SetPen(wxPen(wxColour(60, 60, 60), 2));
    dc.SetBrush(wxBrush(wxColour(235, 235, 235)));
    dc.DrawRoundedRectangle(cx - s / 8, s / 12, s / 4, s * 2 / 3, s / 8);
    dc.SetBrush(wxBrush(wxColour(200, 30, 30)));
    dc.DrawCircle(cx, s - s / 5, s / 5);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(cx - s / 16, s / 3, s / 8, s / 2);
  }
  bmp.SetMask(new wxMask(bmp, *wxWHITE));
  return bmp;
}

// NMEA-Prüfsumme kontrollieren (ohne Prüfsumme wird der Satz akzeptiert)
bool ChecksumOk(const wxString& s) {
  int star = s.Find('*');
  if (star == wxNOT_FOUND) return true;
  unsigned cs = 0;
  for (int i = 1; i < star; ++i) cs ^= static_cast<unsigned>(s[i].GetValue()) & 0xFF;
  long given = -1;
  if (!s.Mid(star + 1, 2).ToLong(&given, 16)) return false;
  return static_cast<long>(cs) == given;
}

// Temperatur aus einem XDR-Satz lesen. Ein XDR-Satz kann mehrere
// Vierergruppen enthalten: Typ, Wert, Einheit, Name.
bool ParseXdrTemperature(const wxString& raw, const wxString& wanted, double& out) {
  wxString s = raw;
  s.Trim();
  s.Trim(false);
  if (s.Len() < 7 || s[0] != '$' || s.Mid(3, 3) != "XDR") return false;
  if (!ChecksumOk(s)) return false;

  int star = s.Find('*');
  if (star != wxNOT_FOUND) s = s.Left(star);

  wxArrayString f = wxStringTokenize(s, ",", wxTOKEN_RET_EMPTY_ALL);
  for (size_t i = 1; i + 3 < f.GetCount(); i += 4) {
    if (f[i] != "C") continue;  // nur Temperaturen
    if (!wanted.IsEmpty() && !f[i + 3].IsSameAs(wanted, false)) continue;

    double v;
    if (!f[i + 1].ToCDouble(&v)) continue;  // locale-unabhängig
    wxString unit = f[i + 2].Upper();
    if (unit == "F")
      v = (v - 32.0) * 5.0 / 9.0;
    else if (unit == "K")
      v -= 273.15;

    out = v;
    return true;
  }
  return false;
}

wxString DefaultSoundFile() {
  wxString* base = GetpSharedDataLocation();
  if (!base) return wxEmptyString;
  wxFileName fn(*base, "");
  fn.AppendDir("sounds");
  fn.SetFullName("beep_ssl.wav");
  return fn.GetFullPath();
}

}  // namespace

// --- Plugin -----------------------------------------------------------------

exhaust_pi::exhaust_pi(void* ppimgr) : opencpn_plugin_116(ppimgr) {}

int exhaust_pi::Init() {
  m_parent = GetOCPNCanvasWindow();
  LoadConfig();
  m_logic.SetParams(m_cfg.alarm);

  m_toolId = InsertPlugInTool("", GetPlugInBitmap(), GetPlugInBitmap(), wxITEM_CHECK,
                              U8("Abgastemperatur"), "", nullptr, -1, 0, this);

  m_timer.reset(new PiTimer([this] { OnTick(); }));
  m_timer->Start(1000);

  if (m_showWindow) ShowWindow(true);

  return WANTS_NMEA_SENTENCES | WANTS_TOOLBAR_CALLBACK | INSTALLS_TOOLBAR_TOOL |
         WANTS_PREFERENCES | WANTS_CONFIG;
}

bool exhaust_pi::DeInit() {
  if (m_timer) {
    m_timer->Stop();
    m_timer.reset();
  }
  if (m_window) {
    m_winPos = m_window->GetPosition();
    // Sofort löschen, NICHT Destroy(): Destroy() verschiebt das Löschen von
    // Fenstern auf später. OpenCPN entlädt die Plugin-DLL aber direkt nach
    // DeInit() – das spätere Löschen würde dann Code aus der bereits
    // entladenen DLL aufrufen und OpenCPN abstürzen lassen.
    m_window->Hide();
    delete m_window;
    m_window = nullptr;
  }
  SaveConfig();
  return true;
}

wxBitmap* exhaust_pi::GetPlugInBitmap() {
  if (!m_icon) m_icon.reset(new wxBitmap(MakeIcon(32)));
  return m_icon.get();
}

wxString exhaust_pi::GetCommonName() { return "ExhaustTemp"; }

wxString exhaust_pi::GetShortDescription() {
  return U8("Abgastemperatur-Anzeige mit Vorwarnung, Alarm und Anstiegsüberwachung");
}

wxString exhaust_pi::GetLongDescription() {
  return U8(
      "Liest die Abgastemperatur aus NMEA-0183-XDR-Sätzen (z. B. "
      "$YDXDR,P,101320,P,Baro,C,34.2,C,EXHAUST*31) und zeigt sie als Zahl an.\n"
      "Überwacht werden: eine Vorwarnstufe (z. B. Teildefekt des Impellers), "
      "ein Alarmwert sowie die Anstiegsgeschwindigkeit oberhalb der minimalen "
      "Betriebstemperatur (z. B. Totalausfall des Impellers). Optional wird auch "
      "ein Datenausfall gemeldet.");
}

void exhaust_pi::SetNMEASentence(wxString& sentence) {
  double t;
  if (!ParseXdrTemperature(sentence, m_cfg.xdrName, t)) return;

  m_temp = t;
  m_haveValue = true;
  m_lastRx = wxGetUTCTimeMillis();
  if (m_dataLost) {
    m_dataLost = false;
    m_lossAcked = false;
    wxLogMessage("exhaust_pi: Daten wieder vorhanden");
  }
  HandleEvents(m_logic.AddSample(m_lastRx.ToDouble() / 1000.0, m_temp));
  RefreshWindow();
}

void exhaust_pi::HandleEvents(const ExhaustLogic::Events& ev) {
  const LogicParams& p = m_cfg.alarm;
  if (ev.alarmRaised) {
    if (m_logic.GetCause() == AlarmCause::Rate)
      wxLogMessage(U8("exhaust_pi: ALARM schneller Anstieg %.1f °C/min bei %.1f °C"),
                   m_logic.Rate(), m_temp);
    else
      wxLogMessage(U8("exhaust_pi: ALARM %.1f °C > Alarmwert %.1f °C"), m_temp, p.limit);
    ShowWindow(true);  // Fenster bei Alarm immer einblenden
    if (m_cfg.soundEnabled) PlayAlarmSound();
    m_tick = 0;
  } else if (ev.warnRaised) {
    wxLogMessage(U8("exhaust_pi: VORWARNUNG %.1f °C > %.1f °C"), m_temp, p.warn);
    ShowWindow(true);
    if (m_cfg.soundEnabled) PlayAlarmSound();  // einmaliger Ton
  } else if (ev.cleared) {
    wxLogMessage(U8("exhaust_pi: Alarm/Vorwarnung beendet, %.1f °C"), m_temp);
  }
}

// Wird jede Sekunde aufgerufen: Datenausfall erkennen, Alarmton wiederholen
void exhaust_pi::OnTick() {
  ++m_tick;

  if (m_haveValue && !m_dataLost &&
      (wxGetUTCTimeMillis() - m_lastRx) > wxLongLong(m_cfg.timeoutSec) * 1000) {
    m_dataLost = true;
    m_lossAcked = false;
    m_logic.ResetHistory();  // Anstieg nach Datenlücke neu berechnen
    wxLogMessage("exhaust_pi: Datenausfall, seit %d s keine Temperatur empfangen",
                 m_cfg.timeoutSec);
    if (m_cfg.alarmOnLoss) {
      ShowWindow(true);
      if (m_cfg.soundEnabled) PlayAlarmSound();
    }
    m_tick = 0;
    RefreshWindow();
  }

  if (!m_cfg.soundEnabled) return;
  const bool alarmActive = m_logic.GetLevel() == Level::Alarm && !m_logic.IsAcked();
  if (alarmActive && !m_dataLost && m_tick % 3 == 0)
    PlayAlarmSound();
  else if (m_dataLost && m_cfg.alarmOnLoss && !m_lossAcked && m_tick % 10 == 0)
    PlayAlarmSound();
}

void exhaust_pi::PlayAlarmSound() {
  wxString f = m_cfg.soundFile.IsEmpty() ? DefaultSoundFile() : m_cfg.soundFile;
  if (!f.IsEmpty() && wxFileExists(f))
    PlugInPlaySoundEx(f);
  else
    wxBell();
}

void exhaust_pi::AcknowledgeAlarm() {
  m_logic.Acknowledge();
  if (m_dataLost) m_lossAcked = true;
  RefreshWindow();
}

DisplayState exhaust_pi::MakeDisplayState() const {
  DisplayState st;
  st.haveValue = m_haveValue;
  st.dataLost = m_dataLost;
  st.temp = m_temp;
  st.params = m_cfg.alarm;
  st.level = m_logic.GetLevel();
  st.cause = m_logic.GetCause();
  st.acked = m_logic.IsAcked();
  st.rateArmed = m_haveValue && m_logic.RateArmed(m_temp);
  st.rateValid = m_logic.RateValid();
  st.rate = m_logic.Rate();
  st.ackPossible = (st.level == Level::Alarm && !st.acked) ||
                   (m_dataLost && m_cfg.alarmOnLoss && !m_lossAcked);
  return st;
}

// --- Fenster ----------------------------------------------------------------

void exhaust_pi::ShowWindow(bool show) {
  if (show && !m_window) {
    m_window = new ExhaustWindow(m_parent, this);
    if (m_winPos != wxDefaultPosition && wxDisplay::GetFromPoint(m_winPos) != wxNOT_FOUND)
      m_window->Move(m_winPos);
    else
      m_window->CentreOnParent();
  }
  if (m_window) {
    m_window->Show(show);
    if (show) RefreshWindow();
  }
  m_showWindow = show;
  SetToolbarItemState(m_toolId, show);
}

void exhaust_pi::RefreshWindow() {
  if (m_window && m_window->IsShown()) m_window->UpdateDisplay(MakeDisplayState());
}

void exhaust_pi::OnWindowClosed() {
  if (m_window) m_winPos = m_window->GetPosition();
  m_showWindow = false;
  SetToolbarItemState(m_toolId, false);
}

void exhaust_pi::OnToolbarToolCallback(int) {
  ShowWindow(!(m_window && m_window->IsShown()));
}

void exhaust_pi::SetColorScheme(PI_ColorScheme) { RefreshWindow(); }

// --- Einstellungen ----------------------------------------------------------

void exhaust_pi::ShowPreferencesDialog(wxWindow* parent) {
  PrefsDialog dlg(parent, m_cfg);
  if (dlg.ShowModal() != wxID_OK) return;
  m_cfg = dlg.GetSettings();
  SaveConfig();
  m_logic.SetParams(m_cfg.alarm);
  m_logic.ResetHistory();
  RefreshWindow();
}

void exhaust_pi::LoadConfig() {
  wxFileConfig* c = GetOCPNConfigObject();
  if (!c) return;
  c->SetPath(kConfigPath);
  LogicParams& a = m_cfg.alarm;
  c->Read("WarnLevel", &a.warn, 50.0);
  c->Read("Limit", &a.limit, 60.0);
  c->Read("Hysteresis", &a.hysteresis, 5.0);
  c->Read("MinOpTemp", &a.minOpTemp, 40.0);
  c->Read("RateLimit", &a.rateLimit, 5.0);
  c->Read("XdrName", &m_cfg.xdrName, "EXHAUST");
  c->Read("TimeoutSec", &m_cfg.timeoutSec, 10);
  c->Read("SoundEnabled", &m_cfg.soundEnabled, true);
  c->Read("AlarmOnLoss", &m_cfg.alarmOnLoss, true);
  c->Read("SoundFile", &m_cfg.soundFile, "");
  c->Read("ShowWindow", &m_showWindow, true);
  int x = -1, y = -1;
  c->Read("WinX", &x, -1);
  c->Read("WinY", &y, -1);
  m_winPos = (x >= 0 && y >= 0) ? wxPoint(x, y) : wxDefaultPosition;
}

void exhaust_pi::SaveConfig() {
  wxFileConfig* c = GetOCPNConfigObject();
  if (!c) return;
  c->SetPath(kConfigPath);
  c->Write("WarnLevel", m_cfg.alarm.warn);
  c->Write("Limit", m_cfg.alarm.limit);
  c->Write("Hysteresis", m_cfg.alarm.hysteresis);
  c->Write("MinOpTemp", m_cfg.alarm.minOpTemp);
  c->Write("RateLimit", m_cfg.alarm.rateLimit);
  c->Write("XdrName", m_cfg.xdrName);
  c->Write("TimeoutSec", m_cfg.timeoutSec);
  c->Write("SoundEnabled", m_cfg.soundEnabled);
  c->Write("AlarmOnLoss", m_cfg.alarmOnLoss);
  c->Write("SoundFile", m_cfg.soundFile);
  c->Write("ShowWindow", m_showWindow);
  c->Write("WinX", m_winPos.x);
  c->Write("WinY", m_winPos.y);
  c->Flush();
}
