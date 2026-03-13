// ============================================================================
// MainFrame.cpp — Root application window implementation
//
// All panels are constructed once in the constructor and stored as members.
// The wxSimplebook (book) switches between them with no animation.
//
// NavigateTo() is the single choke-point for page transitions; add any
// per-page setup/teardown logic there.
// ============================================================================

#include "MainFrame.h"

#include <wx/simplebook.h>

#include "UIColors.h"
#include "LoginPanel.h"
#include "DataInstructionsPanel.h"
#include "BlendFeedPanel.h"
#include "UserPanel.h"
#include "BlendCreationPanel.h"
#include "BlendChatPanel.h"
#include "../AppLayer/AppController.h"
#include "../AppLayer/AppState.h"

// ── Construction ─────────────────────────────────────────────────────────────

MainFrame::MainFrame(AppController& controller)
    : wxFrame(nullptr, wxID_ANY, "YTBLND",
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE | wxMAXIMIZE)
    , controller(controller)
{
    SetBackgroundColour(UIColors::Background);

    // Full-window page switcher — no visible tabs
    book = new wxSimplebook(this, wxID_ANY);
    book->SetBackgroundColour(UIColors::Background);

    // NavigateFn passed to all sub-panels so they can switch pages
    NavigateFn nav = [this](Page p) { NavigateTo(p); };

    // ── Build pages — MUST match Page enum values exactly ─────────────────────

    // Page::LOGIN (0) — first screen shown on launch
    loginPanel = new LoginPanel(book, controller, nav);
    book->AddPage(loginPanel, "Login");

    // Page::DATA_INSTRUCTIONS (1) — shown after login when user has no data
    dataInstrPanel = new DataInstructionsPanel(book, controller, nav);
    book->AddPage(dataInstrPanel, "DataInstructions");

    // Page::BLEND_CREATION (2)
    creationPanel = new BlendCreationPanel(book, controller, nav);
    book->AddPage(creationPanel, "BlendCreation");

    // Page::HOME (3) — blend feed
    // feedPanel must be constructed before BuildHomePage() so it can be reparented
    feedPanel = new BlendFeedPanel(book, controller);
    book->AddPage(BuildHomePage(), "Home");

    // Page::USER (4)
    userPanel = new UserPanel(book, controller, nav);
    book->AddPage(userPanel, "User");

    // Page::SETTINGS (5) — stub: back button only
    {
        wxPanel* stub = new wxPanel(book, wxID_ANY);
        stub->SetBackgroundColour(UIColors::Background);
        auto* vbox = new wxBoxSizer(wxVERTICAL);
        auto* back = new wxButton(stub, wxID_ANY, "< Back");
        back->SetBackgroundColour(UIColors::Surface);
        back->SetForegroundColour(UIColors::TextPrimary);
        back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::HOME); });
        vbox->Add(back, 0, wxALL, 12);
        vbox->Add(new wxStaticText(stub, wxID_ANY, "Settings — coming soon"),
                  0, wxALL, 20);
        stub->SetSizer(vbox);
        settingsPanel = stub;
        book->AddPage(stub, "Settings");
    }

    // Page::BLEND_CHAT (6)
    chatPanel = new BlendChatPanel(book, controller, nav);
    book->AddPage(chatPanel, "BlendChat");

    // ── Outer sizer wraps the book ────────────────────────────────────────────
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(book, 1, wxEXPAND);
    SetSizer(root);

    Maximize(true);
    // Start on login screen
    book->SetSelection(static_cast<int>(Page::LOGIN));
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainFrame::NavigateTo(Page page) {
    // Refresh / reset dynamic panels when navigating to them.
    // Add a case here whenever a new panel needs per-arrival setup.
    if (page == Page::LOGIN)           loginPanel->Reset();
    if (page == Page::BLEND_CREATION)  creationPanel->Reload();
    if (page == Page::HOME)            feedPanel->LoadFromBlend();
    if (page == Page::BLEND_CHAT)      chatPanel->Reload();
    book->SetSelection(static_cast<int>(page));
}

void MainFrame::TriggerFeedRefresh() {
    feedPanel->NextPage();
}

// ── Home page ────────────────────────────────────────────────────────────────

wxPanel* MainFrame::BuildHomePage() {
    auto* page = new wxPanel(book, wxID_ANY);
    page->SetBackgroundColour(UIColors::Background);

    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // ── Top button bar ────────────────────────────────────────────────────────
    auto* topBar = new wxPanel(page, wxID_ANY);
    topBar->SetBackgroundColour(UIColors::Surface);
    auto* hbox = new wxBoxSizer(wxHORIZONTAL);

    auto makeBtn = [&](wxPanel* parent, const wxString& label) {
        auto* btn = new wxButton(parent, wxID_ANY, label,
                                 wxDefaultPosition, wxSize(-1, 36));
        btn->SetBackgroundColour(UIColors::SurfaceRaised);
        btn->SetForegroundColour(UIColors::TextPrimary);
        return btn;
    };

    auto* settingsBtn = makeBtn(topBar, "Settings");
    auto* userBtn     = makeBtn(topBar, "User");
    auto* blendBtn    = makeBtn(topBar, "Blend");

    settingsBtn->Bind(wxEVT_BUTTON, &MainFrame::OnSettings, this);
    userBtn    ->Bind(wxEVT_BUTTON, &MainFrame::OnUser,     this);
    blendBtn   ->Bind(wxEVT_BUTTON, &MainFrame::OnBlend,    this);

    hbox->Add(settingsBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    hbox->AddStretchSpacer(1);
    hbox->Add(userBtn,  0, wxALL | wxALIGN_CENTER_VERTICAL, 8);
    hbox->Add(blendBtn, 0, wxRIGHT | wxTOP | wxBOTTOM | wxALIGN_CENTER_VERTICAL, 8);
    topBar->SetSizer(hbox);

    // ── Title ─────────────────────────────────────────────────────────────────
    auto* titlePanel = new wxPanel(page, wxID_ANY);
    titlePanel->SetBackgroundColour(UIColors::Background);
    auto* titleBox = new wxBoxSizer(wxHORIZONTAL);

    auto* titleLabel = new wxStaticText(titlePanel, wxID_ANY, "YTBLND");
    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(32);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);
    titleLabel->SetForegroundColour(UIColors::Accent);

    titleBox->AddStretchSpacer(1);
    titleBox->Add(titleLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 12);
    titleBox->AddStretchSpacer(1);
    titlePanel->SetSizer(titleBox);

    // ── Blend feed ────────────────────────────────────────────────────────────
    // feedPanel was constructed with book as parent — reparent it to page
    feedPanel->Reparent(page);

    // ── Refresh button ────────────────────────────────────────────────────────
    auto* refreshPanel = new wxPanel(page, wxID_ANY);
    refreshPanel->SetBackgroundColour(UIColors::Background);
    auto* rbox = new wxBoxSizer(wxHORIZONTAL);

    auto* refreshBtn = new wxButton(refreshPanel, wxID_ANY, "Refresh",
                                    wxDefaultPosition, wxSize(120, 36));
    refreshBtn->SetBackgroundColour(UIColors::Accent);
    refreshBtn->SetForegroundColour(UIColors::TextPrimary);
    refreshBtn->Bind(wxEVT_BUTTON, &MainFrame::OnRefresh, this);

    rbox->AddStretchSpacer(1);
    rbox->Add(refreshBtn, 0, wxALL, 12);
    rbox->AddStretchSpacer(1);
    refreshPanel->SetSizer(rbox);

    // ── Assemble vertical layout ──────────────────────────────────────────────
    vbox->Add(topBar,       0, wxEXPAND);
    vbox->Add(titlePanel,   0, wxEXPAND);
    vbox->Add(feedPanel,    1, wxEXPAND | wxALL, 16);
    vbox->Add(refreshPanel, 0, wxEXPAND);
    page->SetSizer(vbox);

    return page;
}

// ── Button handlers ───────────────────────────────────────────────────────────

void MainFrame::OnSettings(wxCommandEvent&) {
    NavigateTo(Page::SETTINGS);
}

void MainFrame::OnUser(wxCommandEvent&) {
    NavigateTo(Page::USER);
}

void MainFrame::OnBlend(wxCommandEvent&) {
    // If a blend was already created this session, go straight to chat.
    // Otherwise start the creation flow.
    // TODO: When multiple blends per user are supported, this should open
    //       a blend-picker screen instead.
    if (AppState::getInstance().getActiveBlend() != nullptr)
        NavigateTo(Page::BLEND_CHAT);
    else
        NavigateTo(Page::BLEND_CREATION);
}

void MainFrame::OnRefresh(wxCommandEvent&) {
    feedPanel->NextPage();
}
