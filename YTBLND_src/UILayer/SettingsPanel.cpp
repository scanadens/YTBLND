/**
 * \file SettingsPanel.cpp
 * \brief Implementation for SettingsPanel.
 * \author Jasmine Kumar
 * \author Xavier Wah-Huen Lusetti
 */

// ============================================================================
// SettingsPanel.cpp
// ============================================================================

#include "SettingsPanel.hpp"

#include <wx/statline.h>
#include <wx/choice.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "UIPages.hpp"
#include "TopBar.hpp"
#include "ConfirmationDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/User.hpp"

// -- Construction --------------------------------------------------------------

SettingsPanel::SettingsPanel(wxWindow* parent, AppController& controller, NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Current->Background);

    auto* root = new wxBoxSizer(wxVERTICAL);

    // -- TopBar ----------------------------------------------------------------
    auto* topBar = new TopBar(this, "Settings", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // -- Theme Selection -----------------------------------------------------
    auto* themePanel = new wxPanel(this, wxID_ANY);
    themePanel->SetBackgroundColour(UIColors::Current->Surface);
    auto* themeSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* themeLabel = new wxStaticText(themePanel, wxID_ANY, "App Theme:");
    themeLabel->SetForegroundColour(UIColors::Current->TextPrimary);

    // Theme Options
    wxArrayString choices;
    choices.Add("Dark Mode");
    choices.Add("Light Mode");
    choices.Add("Neon Mode");

    m_themeChoice = new wxChoice(themePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    m_themeChoice->SetSelection(0); // Default to first item
    m_themeChoice->Bind(wxEVT_CHOICE, &SettingsPanel::OnThemeChanged, this);

    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    themePanel->SetSizer(themeSizer);
    
    // Add it to the root sizer with padding
    root->Add(themePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);  

    // -- Divider ---------------------------------------------------------------
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Current->Separator);
    root->Add(divider, 0, wxEXPAND);

    // -- Spacer pushes logout to bottom ----------------------------------------
    root->AddStretchSpacer(1);

    // -- Logout ----------------------------------------------------------------
    auto* logoutBtn = new wxButton(this, wxID_ANY, "Log Out");
    UIButtons::ApplySizeBounds(logoutBtn, ButtonType::FullWidthDanger);
    logoutBtn->SetBackgroundColour(UIColors::Current->Danger);
    logoutBtn->SetForegroundColour(UIColors::Current->TextPrimary);
    root->Add(logoutBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    SetSizer(root);

    logoutBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnLogout, this);
    Bind(wxEVT_SHOW,              &SettingsPanel::OnShow,   this);

}

// -- Private helpers -----------------------------------------------------------

void SettingsPanel::RefreshUserInfo()
{
    User* user = AppState::getInstance().getCurrentUser();
}

// -- Event handlers ------------------------------------------------------------

void SettingsPanel::OnShow(wxShowEvent& evt)
{
    if (evt.IsShown())
        RefreshUserInfo();
    evt.Skip();
}

void SettingsPanel::OnThemeChanged(wxCommandEvent& /*evt*/) {
    int sel = m_themeChoice->GetSelection();
    ThemeType newTheme = static_cast<ThemeType>(sel);

    // Record the old theme index before switching
    int oldIndex = static_cast<int>(UIColors::GetCurrentTheme());

    // Swap the global color palette
    UIColors::SetTheme(newTheme);

    // Notify MainFrame to recolor all panels, passing the old theme index
    m_controller.getEventRouter().dispatch("theme_updated",
        {{"old_theme_index", std::to_string(oldIndex)}});
}

void SettingsPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::LOGIN);
}
