// ============================================================================
// BlendChatPanel.h — Blend chat screen
//
// Displays the chat room associated with the active blend and lets the user
// send new messages.
//
// LAYOUT
// ──────
//   TopBar ("Blend Chat", back → HOME)
//   [if no active blend] — "No active blend." label (full height)
//   [if blend active]    — scrollable message list + input row
//
// MESSAGE LIST
// ────────────
// Reload() reads AppState::getActiveChatRoom()->getMessages() and rebuilds
// all message rows from scratch.  Each row shows:
//   "[userID]: message text"  (secondary + primary colour)
//   HH:MM timestamp           (muted, smaller font)
// Messages are separated by thin wxStaticLine rules.  The list auto-scrolls
// to the bottom after each rebuild.
//
// SENDING
// ───────
// DoSend() dispatches "sendMessage" with the current user's ID and the
// trimmed input text, then calls Reload() so the new message appears.
// Both the Send button and pressing Enter in the input field call DoSend().
//
// TODO: "sendMessage" is currently a stub in AppController — messages are
//       added to the in-memory ChatRoom but may not be persisted to SQLite.
//       Wire up AppController::handleSendMessage to save to the database.
// TODO: Real-time updates — currently the list only refreshes on send or
//       manual navigation.  Add a polling timer or network event listener
//       to pick up messages from other users.
// TODO: Show the sender's display name instead of raw userID.
// ============================================================================

#pragma once

#include <wx/wx.h>

#include "UIPages.h"

class AppController;
class wxScrolledWindow;
class wxTextCtrl;
class wxStaticText;

class BlendChatPanel : public wxPanel {
public:
    BlendChatPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    // Refreshes message list from AppState's active ChatRoom.
    void Reload();

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    // Scrolled window containing the message rows
    wxScrolledWindow* m_msgScroll;
    // Inner panel inside the scroll window
    wxPanel*          m_msgInner;
    // Sizer for the inner panel
    wxBoxSizer*       m_msgSizer;

    // "No active blend." placeholder (shown when no blend is active)
    wxStaticText*     m_noBlendLabel;

    // Text input for typing messages
    wxTextCtrl*       m_inputCtrl;

    // Panel that wraps the input row (hidden when no blend active)
    wxPanel*          m_inputPanel;

    // Send logic (shared between button click and Enter key)
    void DoSend();

    // Event handlers
    void OnSend(wxCommandEvent& evt);
    void OnInputEnter(wxCommandEvent& evt);
};
