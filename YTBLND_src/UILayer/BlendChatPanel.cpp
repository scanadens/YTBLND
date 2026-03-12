// ============================================================================
// BlendChatPanel.cpp — Blend chat screen implementation
// ============================================================================

#include "BlendChatPanel.h"

#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "UIColors.h"
#include "UIPages.h"
#include "TopBar.h"
#include "ConfirmationDialog.h"
#include "../AppLayer/AppController.h"
#include "../AppLayer/AppState.h"
#include "../AppLayer/EventRouter.h"
#include "../ModelLayer/ChatRoom.h"
#include "../ModelLayer/Message.h"
#include "../ModelLayer/Blend.h"

// ── Helper: format a Unix timestamp as HH:MM ─────────────────────────────────

static wxString FormatTimestamp(std::time_t ts)
{
    std::tm* tm = std::localtime(&ts);
    if (!tm) return "??:??";
    char buf[6];
    std::strftime(buf, sizeof(buf), "%H:%M", tm);
    return wxString::FromAscii(buf);
}

// ── Construction ──────────────────────────────────────────────────────────────

BlendChatPanel::BlendChatPanel(wxWindow* parent,
                               AppController& controller,
                               NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background);

    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar (title updated in Reload) ──────────────────────────────────────
    auto* topBar = new TopBar(this, "Blend Chat", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // ── "No active blend." placeholder ───────────────────────────────────────
    m_noBlendLabel = new wxStaticText(this, wxID_ANY, "No active blend.",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxALIGN_CENTER_HORIZONTAL);
    m_noBlendLabel->SetForegroundColour(UIColors::TextMuted);
    root->Add(m_noBlendLabel, 1, wxALIGN_CENTER | wxALL, 20);

    // ── Message scroll area ───────────────────────────────────────────────────
    m_msgScroll = new wxScrolledWindow(this, wxID_ANY,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxVSCROLL | wxBORDER_NONE);
    m_msgScroll->SetBackgroundColour(UIColors::Surface);
    m_msgScroll->SetScrollRate(0, 12);

    m_msgInner = new wxPanel(m_msgScroll, wxID_ANY);
    m_msgInner->SetBackgroundColour(UIColors::Surface);
    m_msgSizer = new wxBoxSizer(wxVERTICAL);
    m_msgInner->SetSizer(m_msgSizer);

    auto* msgScrollSizer = new wxBoxSizer(wxVERTICAL);
    msgScrollSizer->Add(m_msgInner, 1, wxEXPAND);
    m_msgScroll->SetSizer(msgScrollSizer);

    root->Add(m_msgScroll, 1, wxEXPAND);

    // ── Input panel ───────────────────────────────────────────────────────────
    m_inputPanel = new wxPanel(this, wxID_ANY);
    m_inputPanel->SetBackgroundColour(UIColors::Surface);
    m_inputPanel->SetMinSize(wxSize(-1, 60));

    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);

    m_inputCtrl = new wxTextCtrl(m_inputPanel, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_PROCESS_ENTER | wxBORDER_SIMPLE);
    m_inputCtrl->SetBackgroundColour(UIColors::SurfaceRaised);
    m_inputCtrl->SetForegroundColour(UIColors::TextPrimary);
    m_inputCtrl->SetHint("Type a message...");

    auto* sendBtn = new wxButton(m_inputPanel, wxID_ANY, "Send",
                                 wxDefaultPosition, wxSize(80, 36));
    sendBtn->SetBackgroundColour(UIColors::Accent);
    sendBtn->SetForegroundColour(UIColors::TextPrimary);

    inputSizer->Add(m_inputCtrl, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 10);
    inputSizer->Add(sendBtn,     0, wxALIGN_CENTER_VERTICAL | wxALL,  10);
    m_inputPanel->SetSizer(inputSizer);

    root->Add(m_inputPanel, 0, wxEXPAND);

    SetSizer(root);

    // ── Bindings ──────────────────────────────────────────────────────────────
    sendBtn->Bind(wxEVT_BUTTON,     &BlendChatPanel::OnSend,        this);
    m_inputCtrl->Bind(wxEVT_TEXT_ENTER, &BlendChatPanel::OnInputEnter, this);
}

// ── Public ────────────────────────────────────────────────────────────────────

void BlendChatPanel::Reload()
{
    AppState& state  = AppState::getInstance();
    Blend*    blend  = state.getActiveBlend();
    ChatRoom* room   = state.getActiveChatRoom();

    bool hasBlend = (blend != nullptr && room != nullptr);

    m_noBlendLabel->Show(!hasBlend);
    m_msgScroll   ->Show(hasBlend);
    m_inputPanel  ->Show(hasBlend);

    if (!hasBlend) {
        Layout();
        return;
    }

    // ── Rebuild message list ──────────────────────────────────────────────────
    m_msgSizer->Clear(true /* delete_windows */);

    const std::list<Message>& msgs = room->getMessages();

    if (msgs.empty()) {
        auto* placeholder = new wxStaticText(m_msgInner, wxID_ANY,
                                             "No messages yet.",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxALIGN_CENTER_HORIZONTAL);
        placeholder->SetForegroundColour(UIColors::TextMuted);
        m_msgSizer->Add(placeholder, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
    } else {
        bool first = true;
        for (const Message& msg : msgs) {
            if (!first) {
                // Separator
                auto* sep = new wxStaticLine(m_msgInner, wxID_ANY);
                sep->SetBackgroundColour(UIColors::Separator);
                m_msgSizer->Add(sep, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
            }
            first = false;

            // Message row container
            auto* rowPanel = new wxPanel(m_msgInner, wxID_ANY);
            rowPanel->SetBackgroundColour(UIColors::Surface);

            auto* rowSizer = new wxBoxSizer(wxVERTICAL);

            // "[userID]: message text" line
            auto* msgLine = new wxBoxSizer(wxHORIZONTAL);

            auto* userLabel = new wxStaticText(rowPanel, wxID_ANY,
                                               wxString::FromUTF8(msg.userID + ": "));
            userLabel->SetForegroundColour(UIColors::TextSecondary);

            auto* textLabel = new wxStaticText(rowPanel, wxID_ANY,
                                               wxString::FromUTF8(msg.text));
            textLabel->SetForegroundColour(UIColors::TextPrimary);
            textLabel->Wrap(600);

            msgLine->Add(userLabel, 0, wxALIGN_TOP);
            msgLine->Add(textLabel, 1, wxALIGN_TOP);
            rowSizer->Add(msgLine, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

            // Timestamp
            auto* tsLabel = new wxStaticText(rowPanel, wxID_ANY,
                                             FormatTimestamp(msg.timestamp));
            tsLabel->SetForegroundColour(UIColors::TextMuted);
            wxFont tf = tsLabel->GetFont();
            tf.SetPointSize(tf.GetPointSize() - 1);
            tsLabel->SetFont(tf);
            rowSizer->Add(tsLabel, 0, wxLEFT | wxBOTTOM, 8);

            rowPanel->SetSizer(rowSizer);
            m_msgSizer->Add(rowPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        }
    }

    m_msgInner->Layout();
    m_msgScroll->FitInside();
    m_msgScroll->Layout();

    // Scroll to bottom
    int xUnit, yUnit;
    m_msgScroll->GetScrollPixelsPerUnit(&xUnit, &yUnit);
    if (yUnit > 0) {
        int vH = m_msgScroll->GetVirtualSize().GetHeight();
        m_msgScroll->Scroll(0, vH / yUnit);
    }

    Layout();
}

// ── Private ───────────────────────────────────────────────────────────────────

void BlendChatPanel::DoSend()
{
    wxString text = m_inputCtrl->GetValue().Trim().Trim(false);
    if (text.IsEmpty()) return;

    AppState& state = AppState::getInstance();
    User*     user  = state.getCurrentUser();

    std::string userID = user ? user->getUserID() : "anonymous";

    // TODO: Confirm that AppController::handleSendMessage persists the message
    //       to SQLite (currently it may only update in-memory ChatRoom).
    m_controller.getEventRouter().dispatch("sendMessage",
        {{"userID", userID},
         {"text",   text.ToStdString()}});

    m_inputCtrl->Clear();
    Reload();
}

void BlendChatPanel::OnSend(wxCommandEvent& /*evt*/)
{
    DoSend();
}

void BlendChatPanel::OnInputEnter(wxCommandEvent& /*evt*/)
{
    DoSend();
}
