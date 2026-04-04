// ============================================================================
// LoginPanel.hpp — Sign In / Register screen
//
// The first screen the user sees.  Contains a two-tab form card (Sign In
// and Register) rendered on top of the dark background with the YTBLND title.
//
// SIGN IN
// -------
//   Dispatches "login" with {userID, password}.  userID == username in this
//   app.  Checks AppState::getCurrentUser() immediately after dispatch (the
//   call is synchronous).  On success, calls ProceedAfterLogin().
//   Displays an inline error label on failure (wrong credentials).
//
// REGISTER
//   ---------
//   Validates locally (non-empty, passwords match, ≥6 chars), then dispatches
//   "register" with {userID, username, email, password}.  On success, calls
//   ProceedAfterLogin().  Displays an inline error label if the username is
//   already taken.
//
// POST-LOGIN ROUTING
// ------------------
//   ProceedAfterLogin() checks whether the user's Watch Later list is empty.
//   Empty → DATA_INSTRUCTIONS (upload CSV first).
//   Non-empty → BLEND_CREATION (data already loaded, skip upload).
//   TODO: This check queries the in-memory YouTubeData object.  If data is
//         stored in SQLite and lazy-loaded, the list may appear empty even for
//         returning users.  Verify that SqliteDataManager loads data into the
//         User object before this check runs.
//
// RESET
// -----
//   Reset() is called by MainFrame::NavigateTo(LOGIN) (i.e. on logout).  It
//   clears all fields and returns to the Sign In tab.
// ============================================================================

#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "UIPages.hpp"

class AppController;

// First screen shown when the app launches.
// Presents a Sign In / Register toggle with the appropriate form fields.
// On success, checks whether the user has YouTube data and navigates to
// DATA_INSTRUCTIONS or BLEND_CREATION accordingly.
class LoginPanel : public wxPanel {
public:
    LoginPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    // Clear all fields and reset to Sign In tab. Called by MainFrame on logout.
    void Reset();

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    // Tab toggle buttons
    wxButton*     m_signinTab;
    wxButton*     m_registerTab;

    // Inner simplebook switches between sign-in and register forms
    wxSimplebook* m_formBook;

    // Sign-in form widgets
    wxTextCtrl*   m_siUsername;
    wxTextCtrl*   m_siPassword;
    wxStaticText* m_siError;

    // Register form widgets
    wxTextCtrl*   m_regUsername;
    wxTextCtrl*   m_regEmail;
    wxTextCtrl*   m_regPassword;
    wxTextCtrl*   m_regConfirm;
    wxStaticText* m_regError;

    void BuildSignInForm  (wxWindow* parent, wxSizer* sizer);
    void BuildRegisterForm(wxWindow* parent, wxSizer* sizer);
    void ShowTab(int index);   // 0 = sign in, 1 = register

    void OnSignIn  (wxCommandEvent&);
    void OnRegister(wxCommandEvent&);

    // After a successful login, decide which page comes next.
    void ProceedAfterLogin();
};
