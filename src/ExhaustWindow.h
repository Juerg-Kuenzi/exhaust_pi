// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#pragma once
// Schwebendes Fenster: nur die Temperatur und die Quittier-Taste

#include <wx/wx.h>

#include "exhaust_types.h"

class exhaust_pi;

class ExhaustWindow : public wxDialog {
public:
  // valueFont: Schrift für die Temperatur (wie die Werte im Dashboard)
  ExhaustWindow(wxWindow* parent, exhaust_pi* pi, const wxFont& valueFont);
  void UpdateDisplay(const DisplayState& st);

private:
  void OnClose(wxCloseEvent& evt);
  void OnAck(wxCommandEvent& evt);

  exhaust_pi* m_pi;
  wxPanel* m_panel;
  wxStaticText* m_value;
  wxButton* m_ack;
};
