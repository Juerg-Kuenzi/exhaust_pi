#include "PrefsDialog.h"

#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

PrefsDialog::PrefsDialog(wxWindow* parent, const ExhaustSettings& s)
    : wxDialog(parent, wxID_ANY, U8("Abgastemperatur – Einstellungen"), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
  auto* grid = new wxFlexGridSizer(2, 6, 12);
  grid->AddGrowableCol(1);
  auto label = [&](const wxString& t) {
    grid->Add(new wxStaticText(this, wxID_ANY, t), 0, wxALIGN_CENTER_VERTICAL);
  };
  auto spin = [&](double min, double max, double val, double inc, int digits) {
    auto* c = new wxSpinCtrlDouble(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, min, max, val, inc);
    c->SetDigits(digits);
    grid->Add(c, 0, wxEXPAND);
    return c;
  };
  const LogicParams& a = s.alarm;

  label(U8("Minimale Betriebstemperatur (°C):"));
  m_minOp = spin(0, 300, a.minOpTemp, 1, 0);
  m_minOp->SetToolTip(U8("Die Anstiegsüberwachung ist erst oberhalb dieser Temperatur aktiv, "
                         "damit das Aufwärmen nach dem Motorstart keinen Alarm auslöst."));

  label(U8("Anstiegsalarm ab (°C/min, 0 = aus):"));
  m_rate = spin(0, 100, a.rateLimit, 0.5, 1);
  m_rate->SetToolTip(U8("Alarm, wenn die Temperatur oberhalb der Betriebstemperatur "
                        "schneller steigt (gemittelt über 30 s). Erkennt z. B. den "
                        "Totalausfall des Impellers vor Erreichen des Alarmwerts."));

  label(U8("Vorwarnung ab (°C):"));
  m_warn = spin(0, 1200, a.warn, 1, 0);
  m_warn->SetToolTip(U8("Gelbe Anzeige und einmaliger Ton, z. B. bei reduziertem "
                        "Kühlwasserfluss (Teildefekt des Impellers)."));

  label(U8("Alarm ab (°C):"));
  m_limit = spin(0, 1200, a.limit, 1, 0);

  label(U8("Hysterese (°C):"));
  m_hyst = spin(0, 200, a.hysteresis, 1, 0);
  m_hyst->SetToolTip(U8("Vorwarnung und Alarm enden erst, wenn die Temperatur um diesen "
                        "Betrag unter den jeweiligen Wert gefallen ist."));

  grid->Add(new wxStaticLine(this), 0, wxEXPAND);
  grid->Add(new wxStaticLine(this), 0, wxEXPAND);

  label(U8("XDR-Name (leer = erster Temperaturwert):"));
  m_name = new wxTextCtrl(this, wxID_ANY, s.xdrName);
  grid->Add(m_name, 0, wxEXPAND);

  label(U8("Datenausfall nach (s):"));
  m_timeout = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 2, 600, s.timeoutSec);
  grid->Add(m_timeout, 0, wxEXPAND);

  label("");
  m_sound = new wxCheckBox(this, wxID_ANY, U8("Akustischer Alarm"));
  m_sound->SetValue(s.soundEnabled);
  grid->Add(m_sound);

  label("");
  m_loss = new wxCheckBox(this, wxID_ANY, U8("Alarm auch bei Datenausfall"));
  m_loss->SetValue(s.alarmOnLoss);
  grid->Add(m_loss);

  label(U8("Alarmton (WAV, leer = Standard):"));
  m_file = new wxFilePickerCtrl(this, wxID_ANY, s.soundFile, U8("Alarmton wählen"),
                                "WAV (*.wav)|*.wav", wxDefaultPosition, wxDefaultSize,
                                wxFLP_OPEN | wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
  grid->Add(m_file, 0, wxEXPAND);

  auto* top = new wxBoxSizer(wxVERTICAL);
  top->Add(grid, 1, wxEXPAND | wxALL, 12);
  top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0,
           wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
  SetSizerAndFit(top);
  CentreOnParent();

  Bind(wxEVT_BUTTON, &PrefsDialog::OnOk, this, wxID_OK);
}

void PrefsDialog::OnOk(wxCommandEvent& evt) {
  wxString err;
  if (m_warn->GetValue() >= m_limit->GetValue())
    err = U8("Die Vorwarnung muss unter dem Alarmwert liegen.");
  else if (m_minOp->GetValue() >= m_warn->GetValue())
    err = U8("Die minimale Betriebstemperatur muss unter der Vorwarnung liegen.");
  else if (m_hyst->GetValue() >= m_warn->GetValue())
    err = U8("Die Hysterese muss kleiner als die Vorwarnung sein.");

  if (!err.IsEmpty()) {
    wxMessageBox(err, U8("Abgastemperatur"), wxOK | wxICON_WARNING, this);
    return;
  }
  evt.Skip();  // Dialog normal mit wxID_OK schließen
}

ExhaustSettings PrefsDialog::GetSettings() const {
  ExhaustSettings s;
  s.alarm.minOpTemp = m_minOp->GetValue();
  s.alarm.rateLimit = m_rate->GetValue();
  s.alarm.warn = m_warn->GetValue();
  s.alarm.limit = m_limit->GetValue();
  s.alarm.hysteresis = m_hyst->GetValue();
  wxString name = m_name->GetValue();
  name.Trim();
  name.Trim(false);
  s.xdrName = name;
  s.timeoutSec = m_timeout->GetValue();
  s.soundEnabled = m_sound->GetValue();
  s.alarmOnLoss = m_loss->GetValue();
  s.soundFile = m_file->GetPath();
  return s;
}
