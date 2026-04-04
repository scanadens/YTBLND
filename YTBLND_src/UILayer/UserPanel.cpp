/**
 * \file UserPanel.cpp
 * \brief Implementation for UserPanel.
 * \author Jasmine Kumar
 */

// ============================================================================
// UserPanel.cpp - Account info and logout screen implementation
// ============================================================================

#include "UserPanel.hpp"

#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/filedlg.h>

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

UserPanel::UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    auto* root = new wxBoxSizer(wxVERTICAL);

    // -- TopBar ----------------------------------------------------------------
    auto* topBar = new TopBar(this, "My Account", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // -- User info section -----------------------------------------------------
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

    // -- Divider ---------------------------------------------------------------
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Separator);
    root->Add(divider, 0, wxEXPAND);

    // -- Spacer pushes buttons to bottom ---------------------------------------
    root->AddStretchSpacer(2);

    infoSizer->Add(m_usernameLabel, 0, wxCENTER | wxTOP, 16);
    infoSizer->Add(m_emailLabel,    0, wxCENTER | wxBOTTOM, 16);
    infoPanel->SetSizer(infoSizer);
    root->Add(infoPanel, 0, wxCENTER | wxBOTTOM);

    root->AddStretchSpacer(1); // space out the buttons and the labels

    // Re-upload YouTube data
    auto* reuploadBtn = new wxButton(this, wxID_ANY, "Re-upload YouTube Data");
    UIButtons::ApplySizeBounds(reuploadBtn, ButtonType::Large);
    reuploadBtn->SetBackgroundColour(UIColors::Accent);
    reuploadBtn->SetForegroundColour(UIColors::TextPrimary);
    root->Add(reuploadBtn, 0, wxEXPAND | wxCENTER | wxBOTTOM, 16);

    // Delete Account
    auto* del_acc = new wxButton(this, wxID_ANY, "Delete Account");
    UIButtons::ApplySizeBounds(del_acc, ButtonType::Large);
    del_acc->SetBackgroundColour(UIColors::Danger);
    del_acc->SetForegroundColour(UIColors::TextPrimary);
    root->Add(del_acc, 0, wxEXPAND | wxCENTER | wxBOTTOM, 16); // keep the button in the center

    m_deletePasswordField = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                           wxDefaultSize, wxTE_PASSWORD | wxTE_PROCESS_ENTER);
    m_deletePasswordField->SetHint("Re-enter password to delete account");
    m_deletePasswordField->SetBackgroundColour(UIColors::SurfaceRaised);
    m_deletePasswordField->SetForegroundColour(UIColors::TextPrimary);
    root->Add(m_deletePasswordField, 0, wxEXPAND | wxCENTER | wxBOTTOM, 8);

    m_confirmDeleteBtn = new wxButton(this, wxID_ANY, "Confirm Delete");
    UIButtons::ApplySizeBounds(m_confirmDeleteBtn, ButtonType::Large);
    m_confirmDeleteBtn->SetBackgroundColour(UIColors::Danger);
    m_confirmDeleteBtn->SetForegroundColour(UIColors::TextPrimary);
    root->Add(m_confirmDeleteBtn, 0, wxEXPAND | wxCENTER | wxBOTTOM, 8);

    m_deleteErrorLabel = new wxStaticText(this, wxID_ANY, "");
    m_deleteErrorLabel->SetForegroundColour(UIColors::Danger);
    root->Add(m_deleteErrorLabel, 0, wxEXPAND | wxCENTER | wxBOTTOM, 12);

    // -- Logout ----------------------------------------------------------------
    auto* logoutBtn = new wxButton(this, wxID_ANY, "Log Out");
    UIButtons::ApplySizeBounds(logoutBtn, ButtonType::Large);
    logoutBtn->SetBackgroundColour(UIColors::Danger);
    logoutBtn->SetForegroundColour(UIColors::TextPrimary);
    root->Add(logoutBtn, 0, wxEXPAND | wxCENTER | wxBOTTOM, 16); // keep the button in the bottom center as well

    // add some extra padding space beneath the buttons
    root->AddStretchSpacer(1);

    SetSizer(root);

    reuploadBtn->Bind(wxEVT_BUTTON, &UserPanel::OnReuploadCSV, this);
    del_acc->Bind(wxEVT_BUTTON, &UserPanel::OnDeleteRequest, this);
    m_confirmDeleteBtn->Bind(wxEVT_BUTTON, &UserPanel::OnConfirmDelete, this);
    m_deletePasswordField->Bind(wxEVT_TEXT_ENTER,
                                &UserPanel::OnConfirmDelete, this);
    logoutBtn->Bind(wxEVT_BUTTON, &UserPanel::OnLogout, this);
    Bind(wxEVT_SHOW,              &UserPanel::OnShow,   this);

    SetDeleteReauthVisible(false);
}

// -- Private helpers -----------------------------------------------------------

void UserPanel::Refresh()
{
    RefreshUserInfo();
}

void UserPanel::RefreshUserInfo()
{
    SetDeleteReauthVisible(false);

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

void UserPanel::SetDeleteReauthVisible(bool isVisible)
{
    if (!isVisible) {
        m_deletePasswordField->Clear();
        m_deleteErrorLabel->SetLabel("");
    }

    m_deletePasswordField->Show(isVisible);
    m_confirmDeleteBtn->Show(isVisible);
    m_deleteErrorLabel->Show(isVisible && !m_deleteErrorLabel->GetLabel().IsEmpty());
    Layout();
}

// -- Event handlers ------------------------------------------------------------

void UserPanel::OnShow(wxShowEvent& evt)
{
    if (evt.IsShown())
        Refresh();
    evt.Skip();
}

void UserPanel::OnDeleteRequest(wxCommandEvent& /*evt*/)
{
    ConfirmationDialog dlg(this,
                           "Delete Account",
                           "Are you sure you want to delete your account? This cannot be undone.",
                           "Yes",
                           "Nevermind");
    if (dlg.ShowModal() != wxID_OK) {
        SetDeleteReauthVisible(false);
        return;
    }

    SetDeleteReauthVisible(true);
    m_deletePasswordField->SetFocus();
}

void UserPanel::OnConfirmDelete(wxCommandEvent& /*evt*/)
{
    User* user = AppState::getInstance().getCurrentUser();
    if (user == nullptr) {
        m_deleteErrorLabel->SetLabel("No active user session.");
        m_deleteErrorLabel->Show();
        Layout();
        return;
    }

    const std::string password = m_deletePasswordField->GetValue().ToStdString();
    if (password.empty()) {
        m_deleteErrorLabel->SetLabel("Password is required to delete your account.");
        m_deleteErrorLabel->Show();
        Layout();
        return;
    }

    m_controller.getEventRouter().dispatch(
        "deleteAccount",
        {{"userID", user->getUserID()}, {"password", password}}
    );

    if (AppState::getInstance().getCurrentUser() == nullptr) {
        SetDeleteReauthVisible(false);
        wxMessageBox("Your account has been deleted successfully.",
                     "Account Deleted",
                     wxOK | wxICON_INFORMATION,
                     this);
        m_nav(Page::LOGIN);
        return;
    }

    m_deletePasswordField->Clear();
    m_deleteErrorLabel->SetLabel("Delete failed. Check password and try again.");
    m_deleteErrorLabel->Show();
    Layout();
}

void UserPanel::OnReuploadCSV(wxCommandEvent& /*evt*/)
{
    User* user = AppState::getInstance().getCurrentUser();
    if (!user) {
        wxMessageBox("No active user session.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }

    wxFileDialog dlg(this, "Select YouTube Export Data", "", "",
                     "Supported files (*.csv;*.html;*.htm)|*.csv;*.html;*.htm|"
                     "CSV files (*.csv)|*.csv|"
                     "HTML files (*.html;*.htm)|*.html;*.htm|"
                     "All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string filePath = dlg.GetPath().ToStdString();

    m_controller.getEventRouter().dispatch("uploadData",
        {{"filePath", filePath}, {"userID", user->getUserID()}});

    std::string uploadError = AppState::getInstance().takePendingUploadError();
    if (!uploadError.empty()) {
        wxMessageBox(uploadError, "Upload Failed", wxOK | wxICON_ERROR, this);
        return;
    }

    wxMessageBox("Your YouTube data has been re-uploaded.\n"
                 "Active blends will be updated with your new data.",
                 "Data Updated", wxOK | wxICON_INFORMATION, this);
}

void UserPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::LOGIN);
}
