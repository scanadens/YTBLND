// ============================================================================
// BlendCreationPanel.hpp — New-blend setup screen
//
// Lets the logged-in user assemble a group of up to 8 participants (by
// username) and then create a blend from their combined YouTube data.
//
// LAYOUT
// ──────
//   TopBar  ("New Blend", back → HOME)
//   Action row: "BLEND" label | Add button | Create Blend button
//   Scrollable user list (wxScrolledWindow)
//   "X / 8 users" count label
//
// USER-ADD FLOW
// ─────────────
//   1. User clicks Add → wxTextEntryDialog prompts for a username.
//   2. Duplicate check against m_addedUsers.
//   3. "lookupUser" event dispatched to AppController (optimistic — user is
//      added to the list regardless of whether the backend finds them).
//      TODO: Wire up AppController::handleLookupUser to validate that the
//            username exists in the database before adding.
//   4. "Create Blend" button is enabled once ≥ 2 users are present.
//
// BLEND CREATION
// ──────────────
//   OnCreate builds a payload {"userID_0": ..., "userID_1": ..., ...} and
//   dispatches "createBlend".  On return it navigates to BLEND_CHAT.
//   TODO: If createBlend fails (e.g. one of the users has no data), display
//         an error instead of navigating away.
//
// RELOAD
// ──────
//   Reload() is called by MainFrame::NavigateTo(BLEND_CREATION).  It clears
//   m_addedUsers so the panel starts fresh each time it is shown.
//   TODO: Pre-populate the current user's own username so they don't have to
//         add themselves manually.
// ============================================================================

#pragma once

#include <wx/wx.h>
#include <string>
#include <vector>

#include "IRefreshablePanel.hpp"
#include "UIPages.hpp"

class AppController;
class wxScrolledWindow;
class wxStaticText;
class wxButton;

class BlendCreationPanel : public wxPanel, public IRefreshablePanel {
public:
    BlendCreationPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    void Refresh() override;

    // Clears user list and resets to empty state.
    void Reload();

    // Test-friendly accessors
    #ifdef TESTING_MODE
    const std::vector<std::string>& GetAddedUsersForTesting() const { 
        return m_addedUsers; 
    }
    wxButton* GetCreateBtnForTesting() const { return m_createBtn; }
    wxStaticText* GetCountLabelForTesting() const { return m_countLabel; }
    void AddUserForTesting(const std::string& username) {
        m_addedUsers.push_back(username);
        RebuildUserList();
        UpdateCountLabel();
        if (m_addedUsers.size() >= 2) m_createBtn->Enable();
    }
    void RemoveUserForTesting(const std::string& username) {
        OnRemoveUser(username);
    }
    void CallOnCreateForTesting() {
        wxCommandEvent evt(wxEVT_BUTTON);
        OnCreate(evt);
    }
    #endif

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

    // Blend name input
    wxTextCtrl*       m_blendNameCtrl;

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
