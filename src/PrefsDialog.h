#pragma once
// Einstellungsdialog (OpenCPN: Optionen -> Plugins -> ExhaustTemp -> Einstellungen)

#include <wx/wx.h>

#include "exhaust_types.h"

class wxSpinCtrl;
class wxSpinCtrlDouble;
class wxFilePickerCtrl;

class PrefsDialog : public wxDialog {
public:
  PrefsDialog(wxWindow* parent, const ExhaustSettings& s);
  ExhaustSettings GetSettings() const;

private:
  void OnOk(wxCommandEvent& evt);

  wxSpinCtrlDouble* m_minOp;
  wxSpinCtrlDouble* m_rate;
  wxSpinCtrlDouble* m_warn;
  wxSpinCtrlDouble* m_limit;
  wxSpinCtrlDouble* m_hyst;
  wxTextCtrl* m_name;
  wxSpinCtrl* m_timeout;
  wxCheckBox* m_sound;
  wxCheckBox* m_loss;
  wxFilePickerCtrl* m_file;
};
