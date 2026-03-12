#pragma once

#include <wx/wx.h>
#include <string>
#include <vector>

#include "UIPages.h"

class AppController;
class wxScrolledWindow;
class wxStaticText;
class wxButton;

class BlendCreationPanel : public wxPanel {
public:
    BlendCreationPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    // Clears user list and resets to empty state.
    void Reload();

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    // The scrolled container that holds user rows
    wxScrolledWindow* m_userListScroll;
    // Inner panel inside the scroll window where rows are placed
    wxPanel*          m_userListInner;
    // Sizer for the inner panel
    wxBoxSizer*       m_userListSizer;

    // "No users added" placeholder label
    wxStaticText*     m_emptyLabel;

    // "X / 8 users" count label
    wxStaticText*     m_countLabel;

    // "Create Blend" button — enabled when >= 2 users added
    wxButton*         m_createBtn;

    // Currently added usernames
    std::vector<std::string> m_addedUsers;

    // Rebuilds the user-row display from m_addedUsers.
    void RebuildUserList();

    // Updates the count label text.
    void UpdateCountLabel();

    // Event handlers
    void OnAdd(wxCommandEvent& evt);
    void OnCreate(wxCommandEvent& evt);
    void OnRemoveUser(const std::string& username);
};
