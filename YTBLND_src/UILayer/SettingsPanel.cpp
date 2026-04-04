/**
 * \file SettingsPanel.cpp
 * \brief Implementation for SettingsPanel.
 * \author Jasmine Kumar
 * \author Xavier Lusetti
 */

#include "SettingsPanel.hpp"

#include <wx/statline.h>
#include <wx/choice.h>

#include "UIColors.hpp"
#include "UIPages.hpp"
#include "TopBar.hpp"
#include "ButtonsConfig.hpp"
#include "ConfirmationDialog.hpp"

#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/User.hpp"

// Constructor

SettingsPanel::SettingsPanel(wxWindow* parent, AppController& controller, NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background());

    auto* root = new wxBoxSizer(wxVERTICAL);

    // Top bar
    auto* topBar = new TopBar(this, "Settings", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // Theme selection section
    auto* themePanel = new wxPanel(this, wxID_ANY);
    themePanel->SetBackgroundColour(UIColors::Danger());
    auto* themeSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* themeLabel = new wxStaticText(themePanel, wxID_ANY, "App Theme:");
    themeLabel->SetForegroundColour(UIColors::TextPrimary());

    // Theme Options, populated using the ordered theme vector
    wxArrayString choices;
    for (const wxString& name : UIColors::ThemeOrder) {
            choices.Add(name);
    }

    m_themeChoice = new wxChoice(themePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    
    // Set current selection based on the active palette name
    if (UIColors::Current) {
       m_themeChoice->SetStringSelection(UIColors::Current->Name);
    }

    m_themeChoice->Bind(wxEVT_CHOICE, &SettingsPanel::OnThemeChanged, this);

    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    themeSizer->Add(m_themeChoice, 1, wxEXPAND);

    themePanel->SetSizer(themeSizer);
    
    // Add it to the root sizer with padding
    root->Add(themePanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);  

    // Divider
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Separator());
    root->Add(divider, 0, wxEXPAND);

    // Spacer
    root->AddStretchSpacer(1);

    // Logout section
    auto* logoutBtn = new wxButton(this, wxID_ANY, "Log Out");
    UIButtons::ApplySizeBounds(logoutBtn, ButtonType::FullWidthDanger);
    logoutBtn->SetBackgroundColour(UIColors::Danger());
    logoutBtn->SetForegroundColour(UIColors::TextPrimary());
    root->Add(logoutBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    SetSizer(root);

    logoutBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnLogout, this);
    Bind(wxEVT_SHOW,              &SettingsPanel::OnShow,   this);
}

// Private helpers

void SettingsPanel::RefreshUserInfo()
{
    User* user = AppState::getInstance().getCurrentUser();
}

// Event handlers

void SettingsPanel::OnShow(wxShowEvent& evt)
{
    if (evt.IsShown()) {
        RefreshUserInfo();
        // Update selection in case the theme was changed elsewhere
        if (m_themeChoice && UIColors::Current) {
            m_themeChoice->SetStringSelection(UIColors::Current->Name);
        }
    }
    evt.Skip();
}

void SettingsPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::LOGIN);
}

// When the user changes their desired theme,
// this change needs to be populated throughout the entire app
void SettingsPanel::OnThemeChanged(wxCommandEvent& evt) {
    // Get the string name directly from the choice UI
    wxString selectedTheme = m_themeChoice->GetStringSelection();

    // Update theme selection in the global color controller
    UIColors::SetTheme(selectedTheme);

    // Refresh the entire app starting from the top frame
    wxWindow* top = wxGetTopLevelParent(this);
    UIColors::UpdateUI(top);

    // BROADCASTER for MainFrame to listen for the theme update
    m_controller.getEventRouter().dispatch("theme_updated", {});
}