#pragma once
// Schwebendes Fenster mit großer Temperaturanzeige und Quittier-Taste

#include <wx/wx.h>

#include "exhaust_types.h"

class exhaust_pi;

class ExhaustWindow : public wxDialog {
public:
  ExhaustWindow(wxWindow* parent, exhaust_pi* pi);
  void UpdateDisplay(const DisplayState& st);

private:
  void OnClose(wxCloseEvent& evt);
  void OnAck(wxCommandEvent& evt);

  exhaust_pi* m_pi;
  wxPanel* m_panel;
  wxStaticText* m_value;
  wxStaticText* m_rate;
  wxStaticText* m_info;
  wxStaticText* m_status;
  wxButton* m_ack;
};
