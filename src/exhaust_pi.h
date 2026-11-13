// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Juerg Kuenzi

#pragma once
// OpenCPN-Plugin: Abgastemperatur anzeigen und überwachen

#include <functional>
#include <memory>

#include <wx/wx.h>

#include "ocpn_plugin.h"
#include "exhaust_types.h"

class ExhaustWindow;

// Kleiner Timer, der eine Callback-Funktion aufruft
class PiTimer : public wxTimer {
public:
  explicit PiTimer(std::function<void()> cb) : m_cb(std::move(cb)) {}
  void Notify() override { m_cb(); }

private:
  std::function<void()> m_cb;
};

class exhaust_pi : public opencpn_plugin_116 {
public:
  explicit exhaust_pi(void* ppimgr);
  ~exhaust_pi() override = default;

  // --- opencpn_plugin ---
  int Init() override;
  bool DeInit() override;

  int GetAPIVersionMajor() override { return 1; }
  int GetAPIVersionMinor() override { return 16; }
  int GetPlugInVersionMajor() override { return 1; }
  int GetPlugInVersionMinor() override { return 1; }

  wxBitmap* GetPlugInBitmap() override;
  wxString GetCommonName() override;
  wxString GetShortDescription() override;
  wxString GetLongDescription() override;

  void SetNMEASentence(wxString& sentence) override;

  int GetToolbarToolCount() override { return 1; }
  void OnToolbarToolCallback(int id) override;
  void ShowPreferencesDialog(wxWindow* parent) override;
  void SetColorScheme(PI_ColorScheme cs) override;

  // --- Aufrufe aus dem Anzeigefenster ---
  void AcknowledgeAlarm();
  void OnWindowClosed();

private:
  void LoadConfig();
  void SaveConfig();
  void ShowWindow(bool show);
  void RefreshWindow();
  void HandleEvents(const ExhaustLogic::Events& ev);
  void OnTick();
  void PlayAlarmSound();
  DisplayState MakeDisplayState() const;

  wxWindow* m_parent = nullptr;
  ExhaustWindow* m_window = nullptr;
  std::unique_ptr<PiTimer> m_timer;
  std::unique_ptr<wxBitmap> m_icon;
  int m_toolId = -1;

  ExhaustSettings m_cfg;
  bool m_showWindow = true;
  wxPoint m_winPos = wxDefaultPosition;

  // Messwert- und Alarmzustand
  ExhaustLogic m_logic;
  double m_temp = 0.0;
  bool m_haveValue = false;
  bool m_dataLost = false;
  bool m_lossAcked = false;
  wxLongLong m_lastRx = 0;
  unsigned m_tick = 0;
};
