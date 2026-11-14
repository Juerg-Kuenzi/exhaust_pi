// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#include "ExhaustWindow.h"

#include "exhaust_pi.h"
#include "ocpn_plugin.h"

ExhaustWindow::ExhaustWindow(wxWindow* parent, exhaust_pi* pi, const wxFont& valueFont)
    : wxDialog(parent, wxID_ANY, U8("Abgastemperatur"), wxDefaultPosition, wxDefaultSize,
               wxCAPTION | wxCLOSE_BOX | wxFRAME_FLOAT_ON_PARENT),
      m_pi(pi) {
  m_panel = new wxPanel(this);
  auto* sz = new wxBoxSizer(wxVERTICAL);

  m_value = new wxStaticText(m_panel, wxID_ANY, "---", wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
  m_value->SetFont(valueFont);
  // Feste Breite, damit das Fenster bei wechselnden Werten nicht springt
  wxSize ext = m_value->GetTextExtent(U8("188.8 °C"));
  m_value->SetMinSize(wxSize(ext.x + 8, ext.y));

  m_ack = new wxButton(m_panel, wxID_ANY, U8("Quittieren"), wxDefaultPosition,
                       wxDefaultSize, wxBU_EXACTFIT);
  m_ack->Enable(false);

  sz->Add(m_value, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 4);
  sz->Add(m_ack, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 4);
  m_panel->SetSizer(sz);

  auto* top = new wxBoxSizer(wxVERTICAL);
  top->Add(m_panel, 1, wxEXPAND);
  SetSizerAndFit(top);

  Bind(wxEVT_CLOSE_WINDOW, &ExhaustWindow::OnClose, this);
  m_ack->Bind(wxEVT_BUTTON, &ExhaustWindow::OnAck, this);
}

void ExhaustWindow::UpdateDisplay(const DisplayState& st) {
  // Grundfarben aus dem aktuellen OpenCPN-Farbschema (Tag/Dämmerung/Nacht)
  wxColour bg, fg;
  if (!GetGlobalColor("DILG1", &bg)) bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
  if (!GetGlobalColor("UBLCK", &fg)) fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

  const LogicParams& p = st.params;
  wxString value = "---";
  wxString status;  // nur als Tooltip, spart Platz

  if (!st.haveValue) {
    status = U8("Warte auf Daten …");
  } else if (st.dataLost) {
    status = wxString::Format(U8("Keine Daten – letzter Wert %.1f °C"), st.temp);
    bg = wxColour(110, 110, 110);
    fg = *wxWHITE;
  } else {
    value = wxString::Format(U8("%.1f °C"), st.temp);
    switch (st.level) {
      case Level::Alarm:
        status = st.cause == AlarmCause::Rate ? U8("ALARM – Temperatur steigt schnell")
                                              : U8("ALARM – Alarmwert überschritten");
        if (st.acked) status += U8(" (quittiert)");
        bg = st.acked ? wxColour(230, 120, 0) : wxColour(200, 0, 0);
        fg = st.acked ? *wxBLACK : *wxWHITE;
        break;
      case Level::Warning:
        status = U8("Vorwarnung – Kühlwasserfluss prüfen");
        bg = wxColour(240, 200, 0);
        fg = *wxBLACK;
        break;
      default:
        status = "OK";
        break;
    }
    if (st.rateArmed && st.rateValid)
      status += wxString::Format(U8("\nAnstieg: %+.1f °C/min"), st.rate);
  }
  status += wxString::Format(U8("\nVorwarnung %.0f °C · Alarm %.0f °C"), p.warn, p.limit);

  if (m_value->GetLabel() != value) m_value->SetLabel(value);
  if (m_value->GetToolTipText() != status) {
    m_value->SetToolTip(status);
    m_panel->SetToolTip(status);
  }
  m_ack->Enable(st.ackPossible);

  m_panel->SetBackgroundColour(bg);
  m_value->SetForegroundColour(fg);
  m_panel->Refresh();
}

void ExhaustWindow::OnClose(wxCloseEvent&) {
  // Nicht zerstören, nur ausblenden – über die Toolbar wieder einblendbar
  Hide();
  m_pi->OnWindowClosed();
}

void ExhaustWindow::OnAck(wxCommandEvent&) { m_pi->AcknowledgeAlarm(); }
