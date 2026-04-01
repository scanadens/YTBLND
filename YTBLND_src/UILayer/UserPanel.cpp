// ============================================================================
// UserPanel.cpp — Account info and logout screen implementation
// ============================================================================

#include "UserPanel.hpp"

#include <wx/statline.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "UIPages.hpp"
#include "TopBar.hpp"
#include "ConfirmationDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/User.hpp"

// ── Construction ──────────────────────────────────────────────────────────────

UserPanel::UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar ────────────────────────────────────────────────────────────────
    auto* topBar = new TopBar(this, "My Account", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // ── User info section ─────────────────────────────────────────────────────
    auto* infoPanel = new wxPanel(this, wxID_ANY);
    infoPanel->SetBackgroundColour(UIColors::Surface);
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);

    m_usernameLabel = new wxStaticText(infoPanel, wxID_ANY, "Username: Not logged in");
    m_usernameLabel->SetForegroundColour(UIColors::TextPrimary);
    wxFont uf = m_usernameLabel->GetFont(); // grab a font
    uf.SetPointSize(24);
    m_usernameLabel->SetFont(uf);

    m_emailLabel = new wxStaticText(infoPanel, wxID_ANY, "Email: ");
    m_emailLabel->SetForegroundColour(UIColors::TextSecondary);
    uf = m_emailLabel->GetFont();
    uf.SetPointSize(16);
    m_emailLabel->SetFont(uf);

    // ── Divider ───────────────────────────────────────────────────────────────
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Separator);
    root->Add(divider, 0, wxEXPAND);

    // ── Spacer pushes buttons to bottom ───────────────────────────────────────
    root->AddStretchSpacer(2);

    infoSizer->Add(m_usernameLabel, 0, wxCENTER | wxTOP, 16);
    infoSizer->Add(m_emailLabel,    0, wxCENTER | wxBOTTOM, 16);
    infoPanel->SetSizer(infoSizer);
    root->Add(infoPanel, 0, wxCENTER | wxBOTTOM);

    root->AddStretchSpacer(1); // space out the buttons and the labels

    // Delete Account
    auto* del_acc = new wxButton(this, wxID_ANY, "Delete Account");
    UIButtons::ApplySizeBounds(del_acc, ButtonType::Large);
    del_acc->SetBackgroundColour(UIColors::Danger);
    del_acc->SetForegroundColour(UIColors::TextPrimary);
    root->Add(del_acc, 0, wxEXPAND | wxCENTER | wxBOTTOM, 16); // keep the button in the center

    // ── Logout ────────────────────────────────────────────────────────────────
    auto* logoutBtn = new wxButton(this, wxID_ANY, "Log Out");
    UIButtons::ApplySizeBounds(logoutBtn, ButtonType::Large);
    logoutBtn->SetBackgroundColour(UIColors::Danger);
    logoutBtn->SetForegroundColour(UIColors::TextPrimary);
    root->Add(logoutBtn, 0, wxEXPAND | wxCENTER | wxBOTTOM, 16); // keep the button in the bottom center as well

    // add some extra padding space beneath the buttons
    root->AddStretchSpacer(1);

    SetSizer(root);

    logoutBtn->Bind(wxEVT_BUTTON, &UserPanel::OnLogout, this);
    Bind(wxEVT_SHOW,              &UserPanel::OnShow,   this);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void UserPanel::Refresh()
{
    RefreshUserInfo();
}

void UserPanel::RefreshUserInfo()
{
    User* user = AppState::getInstance().getCurrentUser();
    if (user) {
        m_usernameLabel->SetLabel("Username: " +
                                  wxString::FromUTF8(user->getUsername()));
        
        // verify email presence
        std::string tmp = user->getEmail();
        std::string email_str;
        if (tmp == "") {email_str = "no provided email at registration...";}
        else {email_str = tmp;}

        m_emailLabel->SetLabel("Email: " +
                               wxString::FromUTF8(email_str));
    } else {
        m_usernameLabel->SetLabel("Not logged in");
        m_emailLabel->SetLabel("");
    }
    Layout();
}

// ── Event handlers ────────────────────────────────────────────────────────────

void UserPanel::OnShow(wxShowEvent& evt)
{
    if (evt.IsShown())
        Refresh();
    evt.Skip();
}

void UserPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::LOGIN);
}
