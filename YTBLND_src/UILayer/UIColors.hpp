// ============================================================================
// UserPanel.hpp — Account info and logout screen
//
// A simple panel that shows the current user's username and email, plus a
// Log Out button at the bottom.
//
// LAYOUT
// ──────
//   TopBar    ("My Account", back → HOME)
//   Info panel (Surface background):
//     Username: <value>   (12pt, TextPrimary)
//     Email:    <value>   (TextSecondary)
//   Separator (wxStaticLine)
//   Spacer (pushes Log Out to bottom)
//   Log Out button (Danger colour, full width)
//
// REFRESH ON SHOW
// ───────────────
//   RefreshUserInfo() is called every time the panel becomes visible (via
//   wxEVT_SHOW) so it always reflects the current AppState.
//
// LOGOUT
// ──────
//   OnLogout: ConfirmationDialog → dispatch("logout") → nav(LOGIN).
//   TODO: If logout clears AppState but not any cached local state (e.g. the
//         active blend), ensure those are also reset here or in the "logout"
//         event handler in AppController.
//
// FUTURE FEATURES
// ───────────────
//   TODO: Add a "Change Password" form.
//   TODO: Show the user's YouTube data status (# of Watch Later videos loaded)
//         and an "Update CSV" button to re-upload fresh data.
//   TODO: Display the user's blend history.
// ============================================================================

#pragma once

#include <wx/wx.h>

#include "IRefreshablePanel.hpp"
#include "UIPages.hpp"

class AppController;
class wxStaticText;
class wxTextCtrl;
class wxButton;

class UserPanel : public wxPanel, public IRefreshablePanel {
public:
    UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav);
    void Refresh() override;

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    // Labels that display the logged-in user's info
    wxStaticText* m_usernameLabel;
    wxStaticText* m_emailLabel;
    wxTextCtrl*   m_deletePasswordField;
    wxStaticText* m_deleteErrorLabel;
    wxButton*     m_confirmDeleteBtn;

    // Refreshes user-info labels from AppState on each show.
    void RefreshUserInfo();

    // Shows or hides the password re-auth controls for account deletion.
    void SetDeleteReauthVisible(bool isVisible);

    // Event handlers
    void OnDeleteRequest(wxCommandEvent& evt);
    void OnConfirmDelete(wxCommandEvent& evt);
    void OnLogout (wxCommandEvent& evt);
    void OnShow   (wxShowEvent&    evt);
};
