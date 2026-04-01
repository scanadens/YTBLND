// ============================================================================
// BlendChatPanel.hpp — Blend chat screen
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
#include <wx/timer.h>

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "IRefreshablePanel.hpp"
#include "UIPages.hpp"
#include "../ServiceLayer/ChatWebSocket.hpp"

class AppController;
class wxScrolledWindow;
class wxTextCtrl;
class wxStaticText;

class BlendChatPanel : public wxPanel, public IRefreshablePanel {
public:
    BlendChatPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    void Refresh() override;

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

    // Purple toggle button that shows/hides the participants dropdown
    wxButton*         m_participantsBtn;
    // Dropdown panel containing the participant list (hidden by default)
    wxPanel*          m_participantsDropdown;
    wxBoxSizer*       m_participantsSizer;

    // Text input for typing messages
    wxTextCtrl*       m_inputCtrl;

    // Panel that wraps the input row (hidden when no blend active)
    wxPanel*          m_inputPanel;

    // ── WebSocket / incoming message handling ─────────────────────────────────

    // Timer that drains m_incoming on the main thread (~500 ms interval).
    wxTimer*          m_pollTimer;

    // Blend ID the WebSocket is currently connected to (empty = not connected).
    std::string       m_wsBlendID;

    // Mutex protecting m_incoming (written by WS background thread, read by timer).
    std::mutex        m_incomingMx;

    // Incoming messages from other participants waiting to be added to ChatRoom.
    // Declared after m_incomingMx; m_ws declared last so it is destroyed first,
    // stopping the background thread before the mutex/vector are torn down.
    std::vector<std::pair<std::string, std::string>> m_incoming;

    // Active WebSocket connection (null when no blend is open).
    std::unique_ptr<ChatWebSocket> m_ws;

    // Connects (or reconnects) the WebSocket for the given blend/user.
    void ConnectWebSocket(const std::string& blendID, const std::string& userID);

    // Send logic (shared between button click and Enter key)
    void DoSend();

    // Event handlers
    void OnSend(wxCommandEvent& evt);
    void OnInputEnter(wxCommandEvent& evt);
    void OnPollTimer(wxTimerEvent& evt);
    void OnParticipantsToggle(wxCommandEvent& evt);
};
