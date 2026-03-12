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
