#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "UIPages.h"

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
