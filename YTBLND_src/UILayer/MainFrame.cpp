#include "MainFrame.hpp"
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/dcbuffer.h>

#include "UIColors.hpp"
#include "LoginPanel.hpp"
#include "DataInstructionsPanel.hpp"
#include "BlendFeedPanel.hpp"
#include "UserPanel.hpp"
#include "BlendCreationPanel.hpp"
#include "BlendChatPanel.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"

// ── Background Panel ─────────────────────────────────────────────────────────
class BackgroundPanel : public wxPanel {
public:
    BackgroundPanel(wxWindow* parent, const wxBitmap& bg)
        : wxPanel(parent), bgBmp(bg)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &BackgroundPanel::OnPaint, this);
    }

private:
    wxBitmap bgBmp;

    void OnPaint(wxPaintEvent&) {
        wxBufferedPaintDC dc(this);
        dc.Clear();
        if (bgBmp.IsOk()) {
            wxSize sz = GetClientSize();
            wxImage img = bgBmp.ConvertToImage();
            if (sz.x > 0 && sz.y > 0) {
                // Scale image to fill the window
                wxBitmap scaled(img.Scale(sz.x, sz.y, wxIMAGE_QUALITY_HIGH));
                dc.DrawBitmap(scaled, 0, 0, true);
            }
        }
    }
};

// ── Construction ─────────────────────────────────────────────────────────────
MainFrame::MainFrame(AppController& controller)
    : wxFrame(nullptr, wxID_ANY, "YTBLND",
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      controller(controller)
{
    // 1. Initialize Image Handlers for JPG/PNG support
    wxInitAllImageHandlers();

    // 2. Load the Background Image (Relative to the folder where you run the app)
    if (!backgroundBmp.LoadFile("resources/checkered_wave_background.jpg", wxBITMAP_TYPE_ANY)) {
        wxLogWarning("Background image not found! Expected: resources/checkered_wave_background.jpg");
    }

    // 3. Create Background Panel first
    auto* bgPanel = new BackgroundPanel(this, backgroundBmp);
    
    // 4. Create the Book as a child of the Background Panel
    book = new wxSimplebook(bgPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
    book->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);

    NavigateFn nav = [this](Page p) { NavigateTo(p); };

    // 5. Initialize Pages as children of 'book'
    loginPanel       = new LoginPanel(book, controller, nav);
    dataInstrPanel   = new DataInstructionsPanel(book, controller, nav);
    creationPanel    = new BlendCreationPanel(book, controller, nav);
    feedPanel        = new BlendFeedPanel(book, controller);
    userPanel        = new UserPanel(book, controller, nav);
    chatPanel        = new BlendChatPanel(book, controller, nav);

    // Apply transparency where supported
    loginPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    dataInstrPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    creationPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    feedPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    userPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    chatPanel->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);

    book->AddPage(loginPanel, "Login");
    book->AddPage(dataInstrPanel, "DataInstructions");
    book->AddPage(creationPanel, "BlendCreation");
    book->AddPage(BuildHomePage(), "Home");
    book->AddPage(userPanel, "User");
    book->AddPage(BuildSettingsPage(), "Settings");
    book->AddPage(chatPanel, "BlendChat");

    // 6. Layout: Book expands inside BackgroundPanel
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(book, 1, wxEXPAND);
    bgPanel->SetSizer(rootSizer);

    // Layout: BackgroundPanel expands inside the Frame
    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(bgPanel, 1, wxEXPAND);
    SetSizer(frameSizer);

    Maximize(true);
    book->SetSelection(static_cast<int>(Page::LOGIN));
}

// ── Navigation ────────────────────────────────────────────────────────────────
void MainFrame::NavigateTo(Page page) {
    if (page == Page::LOGIN)           loginPanel->Reset();
    if (page == Page::BLEND_CREATION)  creationPanel->Reload();
    if (page == Page::HOME)            feedPanel->LoadFromBlend();
    if (page == Page::BLEND_CHAT)      chatPanel->Reload();
    book->SetSelection(static_cast<int>(page));
}

void MainFrame::TriggerFeedRefresh() {
    feedPanel->NextPage();
}

// ── Build Home Page ──────────────────────────────────────────────────────────
wxPanel* MainFrame::BuildHomePage() {
    auto* page = new wxPanel(book, wxID_ANY);
    page->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);

    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* topBar = new wxPanel(page, wxID_ANY);
    auto* hbox = new wxBoxSizer(wxHORIZONTAL);

    auto makeBtn = [&](wxPanel* parent, const wxString& label) {
        auto* btn = new wxButton(parent, wxID_ANY, label, wxDefaultPosition, wxSize(-1,36));
        btn->SetBackgroundColour(UIColors::SurfaceRaised);
        btn->SetForegroundColour(UIColors::TextPrimary);
        return btn;
    };

    auto* settingsBtn = makeBtn(topBar, "Settings");
    auto* userBtn     = makeBtn(topBar, "User");
    auto* blendBtn    = makeBtn(topBar, "Blend");

    settingsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::SETTINGS); });
    userBtn    ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::USER); });
    blendBtn   ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::BLEND_CREATION); });

    hbox->Add(settingsBtn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 8);
    hbox->AddStretchSpacer(1);
    hbox->Add(userBtn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 8);
    hbox->Add(blendBtn, 0, wxALL|wxALIGN_CENTER_VERTICAL, 8);
    topBar->SetSizer(hbox);

    auto* titlePanel = new wxPanel(page, wxID_ANY);
    auto* titleBox = new wxBoxSizer(wxHORIZONTAL);
    auto* titleLabel = new wxStaticText(titlePanel, wxID_ANY, "YTBLND");
    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(32);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);
    titleLabel->SetForegroundColour(UIColors::Accent);
    titleBox->AddStretchSpacer(1);
    titleBox->Add(titleLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 12);
    titleBox->AddStretchSpacer(1);
    titlePanel->SetSizer(titleBox);

    feedPanel->Reparent(page);

    auto* refreshPanel = new wxPanel(page, wxID_ANY);
    auto* rbox = new wxBoxSizer(wxHORIZONTAL);
    wxBitmap refreshBmp("./resources/refresh.png", wxBITMAP_TYPE_PNG);
    if (refreshBmp.IsOk())
        refreshBmp = wxBitmap(refreshBmp.ConvertToImage().Rescale(36,36));
    auto* refreshBtn = new wxBitmapButton(refreshPanel, wxID_ANY, refreshBmp, wxDefaultPosition, wxSize(36,36));
    refreshBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ TriggerFeedRefresh(); });
    rbox->AddStretchSpacer(1);
    rbox->Add(refreshBtn, 0, wxALL, 12);
    rbox->AddStretchSpacer(1);
    refreshPanel->SetSizer(rbox);

    vbox->Add(topBar, 0, wxEXPAND);
    vbox->Add(titlePanel, 0, wxEXPAND);
    vbox->Add(feedPanel, 1, wxEXPAND | wxALL, 16);
    vbox->Add(refreshPanel, 0, wxEXPAND);
    page->SetSizer(vbox);

    return page;
}

// ── Settings page stub ───────────────────────────────────────────────────────
wxPanel* MainFrame::BuildSettingsPage() {
    auto* stub = new wxPanel(book, wxID_ANY);
    stub->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* back = new wxButton(stub, wxID_ANY, "< Back");
    back->SetBackgroundColour(UIColors::Surface);
    back->SetForegroundColour(UIColors::TextPrimary);
    back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::HOME); });

    vbox->Add(back, 0, wxALL, 12);
    vbox->Add(new wxStaticText(stub, wxID_ANY, "Settings — coming soon"),
              0, wxALL, 20);
    stub->SetSizer(vbox);
    return stub;
}