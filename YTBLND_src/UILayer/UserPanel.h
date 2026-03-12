#pragma once

#include <wx/wx.h>

#include "UIPages.h"

class AppController;
class wxStaticText;

class UserPanel : public wxPanel {
public:
    UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    // Labels that display the logged-in user's info
    wxStaticText* m_usernameLabel;
    wxStaticText* m_emailLabel;

    // Refreshes user-info labels from AppState on each show.
    void RefreshUserInfo();

    // Event handlers
    void OnLogout (wxCommandEvent& evt);
    void OnShow   (wxShowEvent&    evt);
};
