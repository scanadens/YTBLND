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

    // pull user information from m_controller
    //std::string username = m_controller

    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar ────────────────────────────────────────────────────────────────
    auto* topBar = new TopBar(this, "My Account", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // ── User info section ─────────────────────────────────────────────────────
    auto* infoPanel = new wxPanel(this, wxID_ANY);
    infoPanel->SetBackgroundColour(UIColors::Surface);
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);

    m_usernameLabel = new wxStaticText(infoPanel, wxID_ANY, "Username: —");
    m_usernameLabel->SetForegroundColour(UIColors::TextPrimary);
    wxFont uf = m_usernameLabel->GetFont();
    uf.SetPointSize(12);
    m_usernameLabel->SetFont(uf);

    m_emailLabel = new wxStaticText(infoPanel, wxID_ANY, "Email: —");
    m_emailLabel->SetForegroundColour(UIColors::TextSecondary);

    infoSizer->Add(m_usernameLabel, 0, wxLEFT | wxTOP, 16);
    infoSizer->Add(m_emailLabel,    0, wxLEFT | wxTOP | wxBOTTOM, 16);
    infoPanel->SetSizer(infoSizer);
    root->Add(infoPanel, 0, wxEXPAND);

    // ── Divider ───────────────────────────────────────────────────────────────
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Separator);
    root->Add(divider, 0, wxEXPAND);

    // ── Spacer pushes buttons to bottom ───────────────────────────────────────
    root->AddStretchSpacer(2);

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

void UserPanel::RefreshUserInfo()
{
    User* user = AppState::getInstance().getCurrentUser();
    if (user) {
        m_usernameLabel->SetLabel("Username: " +
                                  wxString::FromUTF8(user->getUsername()));
        m_emailLabel->SetLabel("Email: " +
                               wxString::FromUTF8(user->getEmail()));
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
        RefreshUserInfo();
    evt.Skip();
}

void UserPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::LOGIN);
}
