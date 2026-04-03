// ============================================================================
// BlendChatPanel.cpp — Blend chat screen implementation
// ============================================================================

#include "BlendChatPanel.hpp"

#include "../ServerConfig.hpp"
#include <wx/scrolwin.h>
#include <ctime>
#include <memory>
#include <mutex>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "UIPages.hpp"
#include "TopBar.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ModelLayer/Message.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ModelLayer/User.hpp"

// ── Helper: format a Unix timestamp as HH:MM ─────────────────────────────────

static wxString FormatTimestamp(std::time_t ts)
{
    std::tm* tm = std::localtime(&ts);
    if (!tm) return "??:??";
    char buf[9]; // "12:34 PM\0"
    std::strftime(buf, sizeof(buf), "%I:%M %p", tm);
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
    SetBackgroundColour(UIColors::Background());

    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar (title updated in Reload) ──────────────────────────────────────
    auto* topBar = new TopBar(this, "Blend Chat", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // ── "No active blend." placeholder ───────────────────────────────────────
    m_noBlendLabel = new wxStaticText(this, wxID_ANY, "No active blend.",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxALIGN_CENTER_HORIZONTAL);
    m_noBlendLabel->SetForegroundColour(UIColors::TextMuted());
    root->Add(m_noBlendLabel, 1, wxALIGN_CENTER | wxALL, 20);

    // ── Participants toggle button + dropdown ─────────────────────────────────
    m_participantsBtn = new wxButton(this, wxID_ANY, "  Participants",
                                     wxDefaultPosition, wxSize(-1, 48),
                                     wxBU_LEFT);
    m_participantsBtn->SetBackgroundColour(UIColors::Accent());
    m_participantsBtn->SetForegroundColour(*wxWHITE);
    {
        wxFont f = m_participantsBtn->GetFont();
        f.SetPointSize(14);
        f.SetWeight(wxFONTWEIGHT_BOLD);
        m_participantsBtn->SetFont(f);
    }
    m_participantsBtn->Hide(); // shown in Reload() when blend is active
    root->Add(m_participantsBtn, 0, wxEXPAND);

    m_participantsDropdown = new wxPanel(this, wxID_ANY);
    m_participantsDropdown->SetBackgroundColour(UIColors::SurfaceRaised());
    m_participantsSizer = new wxBoxSizer(wxVERTICAL);
    m_participantsDropdown->SetSizer(m_participantsSizer);
    m_participantsDropdown->Hide();
    root->Add(m_participantsDropdown, 0, wxEXPAND);

    m_participantsBtn->Bind(wxEVT_BUTTON, &BlendChatPanel::OnParticipantsToggle, this);

    // ── Message scroll area ───────────────────────────────────────────────────
    m_msgScroll = new wxScrolledWindow(this, wxID_ANY,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxVSCROLL | wxBORDER_NONE);
    m_msgScroll->SetBackgroundColour(UIColors::Surface());
    m_msgScroll->SetScrollRate(0, 12);

    m_msgInner = new wxPanel(m_msgScroll, wxID_ANY);
    m_msgInner->SetBackgroundColour(UIColors::Surface());
    m_msgSizer = new wxBoxSizer(wxVERTICAL);
    m_msgInner->SetSizer(m_msgSizer);

    auto* msgScrollSizer = new wxBoxSizer(wxVERTICAL);
    msgScrollSizer->Add(m_msgInner, 1, wxEXPAND);
    m_msgScroll->SetSizer(msgScrollSizer);

    root->Add(m_msgScroll, 1, wxEXPAND);

    // ── Input panel ───────────────────────────────────────────────────────────
    m_inputPanel = new wxPanel(this, wxID_ANY);
    m_inputPanel->SetBackgroundColour(UIColors::Surface());
    m_inputPanel->SetMinSize(wxSize(-1, 60));

    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);

    m_inputCtrl = new wxTextCtrl(m_inputPanel, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_PROCESS_ENTER | wxBORDER_SIMPLE);
    m_inputCtrl->SetBackgroundColour(UIColors::SurfaceRaised());
    m_inputCtrl->SetForegroundColour(UIColors::TextPrimary());
    m_inputCtrl->SetHint("Type a message...");

    auto* sendBtn = new wxButton(m_inputPanel, wxID_ANY, "Send");
    UIButtons::ApplySizeBounds(sendBtn, ButtonType::Nav);
    sendBtn->SetBackgroundColour(UIColors::Accent());
    sendBtn->SetForegroundColour(UIColors::TextPrimary());

    inputSizer->Add(m_inputCtrl, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 10);
    inputSizer->Add(sendBtn,     0, wxALIGN_CENTER_VERTICAL | wxALL,  10);
    m_inputPanel->SetSizer(inputSizer);

    root->Add(m_inputPanel, 0, wxEXPAND);

    SetSizer(root);

    // ── Bindings ──────────────────────────────────────────────────────────────
    sendBtn->Bind(wxEVT_BUTTON,     &BlendChatPanel::OnSend,        this);
    m_inputCtrl->Bind(wxEVT_TEXT_ENTER, &BlendChatPanel::OnInputEnter, this);

    // ── Poll timer — drains incoming WebSocket messages on the main thread ────
    m_pollTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &BlendChatPanel::OnPollTimer, this, m_pollTimer->GetId());
    m_pollTimer->Start(500 /* ms */);

    // ── Logout listener — close WebSocket immediately when user logs out ───────
    m_controller.getEventRouter().registerListener("logout",
        [this](const EventPayload&) {
            m_ws.reset();
            m_wsBlendID.clear();
        });
}

// ── Public ────────────────────────────────────────────────────────────────────

void BlendChatPanel::Refresh()
{
    Reload();
}

void BlendChatPanel::Reload()
{
    AppState& state  = AppState::getInstance();
    Blend*    blend  = state.getActiveBlend();
    ChatRoom* room   = state.getActiveChatRoom();

    bool hasBlend = (blend != nullptr && room != nullptr);

    m_noBlendLabel          ->Show(!hasBlend);
    m_participantsBtn       ->Show(hasBlend);
    m_msgScroll             ->Show(hasBlend);
    m_inputPanel            ->Show(hasBlend);

    if (!hasBlend) {
        m_participantsDropdown->Hide();
        m_ws.reset();
        m_wsBlendID.clear();
        Layout();
        return;
    }

    // ── Connect WebSocket if blend changed or not yet connected ───────────────
    User* currentUser = state.getCurrentUser();
    if (currentUser && room->getBlendID() != m_wsBlendID) {
        // Load history first (synchronous — ChatRoom is populated before UI rebuild)
        m_controller.getEventRouter().dispatch("openChat",
            {{"blendID", room->getBlendID()},
             {"userID",  currentUser->getUserID()}});
        ConnectWebSocket(room->getBlendID(), currentUser->getUserID());
    }

    // ── Populate participants dropdown ────────────────────────────────────────
    m_participantsBtn->Show();
    m_participantsSizer->Clear(true /* delete_windows */);

    for (const User& p : blend->getParticipants()) {
        auto* row = new wxPanel(m_participantsDropdown, wxID_ANY);
        row->SetBackgroundColour(UIColors::SurfaceRaised());

        // Username — bold, slightly larger
        auto* nameLbl = new wxStaticText(row, wxID_ANY,
                                         wxString::FromUTF8(p.getUsername()));
        nameLbl->SetForegroundColour(UIColors::TextPrimary());
        {
            wxFont f = nameLbl->GetFont();
            f.SetPointSize(11);
            f.SetWeight(wxFONTWEIGHT_BOLD);
            nameLbl->SetFont(f);
        }

        // User ID — normal weight, smaller
        auto* idLbl = new wxStaticText(row, wxID_ANY,
                                       wxString::FromUTF8("  (" + p.getUserID() + ")"));
        idLbl->SetForegroundColour(UIColors::TextSecondary());
        {
            wxFont f = idLbl->GetFont();
            f.SetPointSize(9);
            idLbl->SetFont(f);
        }

        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->Add(nameLbl, 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxLEFT, 8);
        rowSizer->Add(idLbl,   0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 8);
        row->SetSizer(rowSizer);

        m_participantsSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 1);
    }

    m_participantsDropdown->Layout();

    // ── Rebuild message list ──────────────────────────────────────────────────
    m_msgSizer->Clear(true /* delete_windows */);

    const std::list<Message>& msgs = room->getMessages();

    if (msgs.empty()) {
        auto* placeholder = new wxStaticText(m_msgInner, wxID_ANY,
                                             "No messages yet.",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxALIGN_CENTER_HORIZONTAL);
        placeholder->SetForegroundColour(UIColors::TextMuted());
        m_msgSizer->Add(placeholder, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 20);
    } else {
        static const wxColour kHoverBg(50, 50, 50);

        for (const Message& msg : msgs) {
            // Message row container
            auto* rowPanel = new wxPanel(m_msgInner, wxID_ANY);
            rowPanel->SetBackgroundColour(UIColors::Surface());

            auto* rowSizer = new wxBoxSizer(wxVERTICAL);

            // ── Header line: [username bold]  [spacer]  [HH:MM AM/PM small grey] ──
            auto* headerLine = new wxBoxSizer(wxHORIZONTAL);

            auto* userLabel = new wxStaticText(rowPanel, wxID_ANY,
                                               wxString::FromUTF8(msg.userID));
            userLabel->SetForegroundColour(UIColors::TextSecondary());
            wxFont uf = userLabel->GetFont();
            uf.SetWeight(wxFONTWEIGHT_BOLD);
            userLabel->SetFont(uf);

            auto* tsLabel = new wxStaticText(rowPanel, wxID_ANY,
                                             FormatTimestamp(msg.timestamp));
            tsLabel->SetForegroundColour(UIColors::TextMuted());
            wxFont tf = tsLabel->GetFont();
            tf.SetPointSize(tf.GetPointSize() - 1);
            tsLabel->SetFont(tf);

            headerLine->Add(userLabel, 0, wxALIGN_CENTER_VERTICAL);
            headerLine->AddStretchSpacer(1);
            headerLine->Add(tsLabel,   0, wxALIGN_CENTER_VERTICAL);
            rowSizer->Add(headerLine, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

            // ── Message text below the header ─────────────────────────────────────
            auto* textLabel = new wxStaticText(rowPanel, wxID_ANY,
                                               wxString::FromUTF8(msg.text));
            textLabel->SetForegroundColour(UIColors::TextPrimary());
            textLabel->Wrap(600);
            rowSizer->Add(textLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

            rowPanel->SetSizer(rowSizer);
            m_msgSizer->Add(rowPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);

            // ── Hover highlight ───────────────────────────────────────────────────
            auto applyHover = [=](bool hover) {
                wxColour bg = hover ? kHoverBg : UIColors::Surface();
                rowPanel ->SetBackgroundColour(bg);
                userLabel->SetBackgroundColour(bg);
                tsLabel  ->SetBackgroundColour(bg);
                textLabel->SetBackgroundColour(bg);
                rowPanel->Refresh();
            };
            for (wxWindow* w : std::initializer_list<wxWindow*>{
                    rowPanel, userLabel, tsLabel, textLabel}) {
                w->Bind(wxEVT_ENTER_WINDOW, [applyHover](wxMouseEvent& e){ applyHover(true);  e.Skip(); });
                w->Bind(wxEVT_LEAVE_WINDOW, [applyHover](wxMouseEvent& e){ applyHover(false); e.Skip(); });
            }
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
    ChatRoom* room  = state.getActiveChatRoom();

    if (!room) return;

    std::string userID  = user ? user->getUserID() : "anonymous";
    std::string textStr = text.ToStdString();

    // Dispatch through AppController — handleSendMessage validates the sender
    // and calls room->addMessage(), making the message available for Reload().
    m_controller.getEventRouter().dispatch("sendMessage",
        {{"userID", userID}, {"text", textStr}});

    // Send to server via WebSocket so other participants receive it.
    if (m_ws) {
        try {
            m_ws->send_message(textStr);
        } catch (const std::exception& e) {
            std::cerr << "[BlendChatPanel] WebSocket send failed: " << e.what() << "\n";
        }
    }

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

void BlendChatPanel::OnParticipantsToggle(wxCommandEvent& /*evt*/)
{
    bool nowVisible = !m_participantsDropdown->IsShown();
    m_participantsDropdown->Show(nowVisible);
    Layout();
}

// ── WebSocket ─────────────────────────────────────────────────────────────────

void BlendChatPanel::ConnectWebSocket(const std::string& blendID,
                                      const std::string& userID)
{
    // Reset any existing connection first.
    m_ws.reset();
    m_wsBlendID.clear();

    try {
        m_ws = std::make_unique<ChatWebSocket>(kAppBackendWsPrefix);
        m_ws->set_blendID(blendID);
        m_ws->set_userID(userID);

        // Register incoming-message callback — runs on the WS background thread.
        // Push to m_incoming; OnPollTimer drains it on the main thread.
        m_ws->setOnMessage([this](const std::string& sender,
                                  const std::string& content) {
            std::lock_guard<std::mutex> lk(m_incomingMx);
            m_incoming.emplace_back(sender, content);
        });

        m_ws->connect();
        m_wsBlendID = blendID;
    } catch (const std::exception& e) {
        std::cerr << "[BlendChatPanel] WebSocket connect failed: " << e.what() << "\n";
        m_ws.reset();
    }
}

void BlendChatPanel::OnPollTimer(wxTimerEvent& /*evt*/)
{
    // Drain incoming messages pushed by the WebSocket background thread.
    std::vector<std::pair<std::string, std::string>> batch;
    {
        std::lock_guard<std::mutex> lk(m_incomingMx);
        std::swap(batch, m_incoming);
    }

    if (batch.empty()) return;

    ChatRoom* room = AppState::getInstance().getActiveChatRoom();
    if (!room) return;

    for (auto& [sender, content] : batch) {
        // Skip echoes of our own messages (already added in DoSend).
        User* me = AppState::getInstance().getCurrentUser();
        if (me && sender == me->getUserID()) continue;
        room->addMessage(sender, content);
    }

    Reload();
}
