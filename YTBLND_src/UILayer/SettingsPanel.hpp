// ============================================================================
// SettingsPanel.hpp — Theme selection and logout screen
// ============================================================================

#pragma once

#include <wx/wx.h>

#include "UIPages.hpp"

class AppController;
class wxChoice;

class SettingsPanel : public wxPanel {
public:
    SettingsPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

private:
    AppController& m_controller;
    NavigateFn     m_nav;
    wxChoice* m_themeChoice = nullptr;

    void RefreshUserInfo();

    void OnShow(wxShowEvent& evt);
    void OnThemeChanged(wxCommandEvent& evt);
    void OnLogout(wxCommandEvent& evt);
};