#include "ExhaustWindow.h"

#include "exhaust_pi.h"
#include "ocpn_plugin.h"

ExhaustWindow::ExhaustWindow(wxWindow* parent, exhaust_pi* pi)
    : wxDialog(parent, wxID_ANY, U8("Abgastemperatur"), wxDefaultPosition, wxDefaultSize,
               wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER | wxFRAME_FLOAT_ON_PARENT),
      m_pi(pi) {
  m_panel = new wxPanel(this);
  auto* sz = new wxBoxSizer(wxVERTICAL);

  m_value = new wxStaticText(m_panel, wxID_ANY, "---", wxDefaultPosition, wxDefaultSize,
                             wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
  m_value->SetFont(wxFont(wxFontInfo(36).Bold()));
  wxSize ext = m_value->GetTextExtent(U8("888.8 °C"));
  m_value->SetMinSize(wxSize(ext.x + 20, ext.y + 4));

  m_rate = new wxStaticText(m_panel, wxID_ANY, "");
  m_info = new wxStaticText(m_panel, wxID_ANY, "");
  m_status = new wxStaticText(m_panel, wxID_ANY, U8("Warte auf Daten …"));
  m_status->SetFont(m_status->GetFont().Bold());
  m_ack = new wxButton(m_panel, wxID_ANY, U8("Alarm quittieren"));
  m_ack->Enable(false);

  sz->Add(m_value, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
  sz->Add(m_rate, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 10);
  sz->Add(m_info, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 10);
  sz->Add(m_status, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 6);
  sz->Add(m_ack, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM, 10);
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
  wxString rate;
  wxString status;

  if (!st.haveValue) {
    status = U8("Warte auf Daten …");
  } else if (st.dataLost) {
    status = wxString::Format(U8("KEINE DATEN – letzter Wert %.1f °C"), st.temp);
    bg = wxColour(110, 110, 110);
    fg = *wxWHITE;
  } else {
    value = wxString::Format(U8("%.1f °C"), st.temp);

    if (p.rateLimit <= 0)
      rate = U8("Anstiegsüberwachung aus");
    else if (!st.rateArmed)
      rate = wxString::Format(U8("Anstieg: unter Betriebstemp. (%.0f °C)"), p.minOpTemp);
    else if (!st.rateValid)
      rate = U8("Anstieg: wird ermittelt …");
    else
      rate = wxString::Format(U8("Anstieg: %+.1f °C/min"), st.rate);

    switch (st.level) {
      case Level::Alarm: {
        const bool isRate = st.cause == AlarmCause::Rate;
        if (!st.acked) {
          status = isRate ? U8("ALARM – Temperatur steigt schnell!")
                          : U8("ALARM – Alarmwert überschritten!");
          bg = wxColour(200, 0, 0);
          fg = *wxWHITE;
        } else {
          status = isRate ? U8("Schneller Anstieg (quittiert)")
                          : U8("Alarmwert überschritten (quittiert)");
          bg = wxColour(230, 120, 0);
          fg = *wxBLACK;
        }
        break;
      }
      case Level::Warning:
        status = U8("VORWARNUNG – Kühlwasserfluss prüfen");
        bg = wxColour(240, 200, 0);
        fg = *wxBLACK;
        break;
      default:
        status = "OK";
        break;
    }
  }
  wxString info = wxString::Format(U8("Vorwarnung %.0f °C · Alarm %.0f °C"), p.warn, p.limit);

  if (m_value->GetLabel() != value) m_value->SetLabel(value);
  if (m_rate->GetLabel() != rate) m_rate->SetLabel(rate);
  if (m_info->GetLabel() != info) m_info->SetLabel(info);
  if (m_status->GetLabel() != status) m_status->SetLabel(status);
  m_ack->Enable(st.ackPossible);

  m_panel->SetBackgroundColour(bg);
  for (wxStaticText* t : {m_value, m_rate, m_info, m_status}) t->SetForegroundColour(fg);
  m_panel->Layout();
  m_panel->Refresh();
}

void ExhaustWindow::OnClose(wxCloseEvent&) {
  // Nicht zerstören, nur ausblenden – über die Toolbar wieder einblendbar
  Hide();
  m_pi->OnWindowClosed();
}

void ExhaustWindow::OnAck(wxCommandEvent&) { m_pi->AcknowledgeAlarm(); }
