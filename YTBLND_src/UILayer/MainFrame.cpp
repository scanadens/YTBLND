#include "MainFrame.hpp"
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "LoginPanel.hpp"
#include "DataInstructionsPanel.hpp"
#include "BlendFeedPanel.hpp"
#include "UserPanel.hpp"
#include "SettingsPanel.hpp"
#include "BlendCreationPanel.hpp"
#include "BlendChatPanel.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"

namespace {
wxString ResolveResourcePath(const wxString& fileName)
{
    const wxArrayString relativeCandidates = {
        "YTBLND_src/resources/" + fileName,
        "resources/" + fileName,
        "../resources/" + fileName,
    };

    for (const auto& candidate : relativeCandidates) {
        if (wxFileExists(candidate)) {
            return candidate;
        }
    }

    wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
    const wxString exeDir = exePath.GetPath();
    const wxArrayString exeRelativeCandidates = {
        exeDir + "/../../YTBLND_src/resources/" + fileName,
        exeDir + "/../resources/" + fileName,
        exeDir + "/resources/" + fileName,
    };

    for (const auto& candidate : exeRelativeCandidates) {
        if (wxFileExists(candidate)) {
            return candidate;
        }
    }

    return "";
}
}

// ── Background Panel ─────────────────────────────────────────────────────────
class ImageBackgroundPanel : public wxPanel {
public:
    ImageBackgroundPanel(wxWindow* parent,
                         const std::unordered_map<int, wxImage>& imageStore,
                         int key)
        : wxPanel(parent)
        , m_imageStore(imageStore)
        , m_key(key)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &ImageBackgroundPanel::OnPaint, this);
    }

private:
    const std::unordered_map<int, wxImage>& m_imageStore;
    int m_key;

    void OnPaint(wxPaintEvent&) {
        wxBufferedPaintDC dc(this);
        dc.Clear();
        auto it = m_imageStore.find(m_key);
        if (it == m_imageStore.end() || !it->second.IsOk()) {
            return;
        }

        wxSize sz = GetClientSize();
        if (sz.x <= 0 || sz.y <= 0) {
            return;
        }

        wxBitmap scaled(it->second.Scale(sz.x, sz.y, wxIMAGE_QUALITY_HIGH));
        dc.DrawBitmap(scaled, 0, 0, true);
    }
};

// ── Construction ─────────────────────────────────────────────────────────────
MainFrame::MainFrame(AppController& controller)
    : wxFrame(nullptr, wxID_ANY, "YTBLND",
              wxDefaultPosition, wxSize(1280, 800),
              wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      controller(controller)
{
    // resolve the path to the background image and load it as a wxImage
    const wxString bgPath = ResolveResourcePath("checkered_wave_background.jpg");
    if (bgPath.empty() || !LoadImage(BG_MAIN, bgPath)) {
        wxLogWarning("Background image not found. Checked ../resources and resources.");
    }
    
    // create the outer container panel with the app background colour
    auto* bgPanel = new wxPanel(this);
    bgPanel->SetBackgroundColour(UIColors::Background);
    
    // placing our new background panel in the book
    book = new wxSimplebook(bgPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

    NavigateFn nav = [this](Page p) { NavigateTo(p); };

    // --- Initialize pages and wrap each in a panel that paints the background image --

    // the login panel — paints its own background image
    loginPanel = new LoginPanel(book, controller, nav, images[BG_MAIN]);
    book->AddPage(loginPanel, "Login");

    auto* dataPageBg = new wxPanel(book);
    dataPageBg->SetBackgroundColour(UIColors::Background);
    dataInstrPanel = new DataInstructionsPanel(dataPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(dataInstrPanel, 1, wxEXPAND);
        dataPageBg->SetSizer(s);
    }
    book->AddPage(dataPageBg, "DataInstructions");

    auto* creationPageBg = new wxPanel(book);
    creationPageBg->SetBackgroundColour(UIColors::Background);
    creationPanel = new BlendCreationPanel(creationPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(creationPanel, 1, wxEXPAND);
        creationPageBg->SetSizer(s);
    }
    book->AddPage(creationPageBg, "BlendCreation");

    auto* homePageBg = new wxPanel(book);
    homePageBg->SetBackgroundColour(UIColors::Background);
    feedPanel = new BlendFeedPanel(homePageBg, controller);
    {
        auto* homePage = BuildHomePage(homePageBg);
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(homePage, 1, wxEXPAND);
        homePageBg->SetSizer(s);
    }
    book->AddPage(homePageBg, "Home");

    auto* userPageBg = new wxPanel(book);
    userPageBg->SetBackgroundColour(UIColors::Background);
    userPanel = new UserPanel(userPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(userPanel, 1, wxEXPAND);
        userPageBg->SetSizer(s);
    }
    book->AddPage(userPageBg, "User");

    auto* settingsPageBg = new wxPanel(book);
    settingsPageBg->SetBackgroundColour(UIColors::Background);
    settingsPanel = new SettingsPanel(settingsPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(settingsPanel, 1, wxEXPAND);
        settingsPageBg->SetSizer(s);
    }
    book->AddPage(settingsPageBg, "Settings");

    auto* chatPageBg = new wxPanel(book);
    chatPageBg->SetBackgroundColour(UIColors::Background);
    chatPanel = new BlendChatPanel(chatPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(chatPanel, 1, wxEXPAND);
        chatPageBg->SetSizer(s);
    }
    book->AddPage(chatPageBg, "BlendChat");

    // 6. Layout: Book expands inside BackgroundPanel
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(book, 1, wxEXPAND);
    bgPanel->SetSizer(rootSizer);

    // Layout: BackgroundPanel expands inside the Frame
    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(bgPanel, 1, wxEXPAND);
    SetSizer(frameSizer);

    Maximize(true);
    book->SetSelection(static_cast<int>(Page::HOME));
    Layout();
    book->SetSelection(static_cast<int>(Page::LOGIN));
}

// ── Navigation ────────────────────────────────────────────────────────────────
void MainFrame::NavigateTo(Page page) {
    if (page == Page::LOGIN)           loginPanel->Refresh();
    if (page == Page::BLEND_CREATION)  creationPanel->Refresh();
    if (page == Page::HOME)            feedPanel->Refresh();
    if (page == Page::USER)            userPanel->Refresh();
    if (page == Page::BLEND_CHAT)      chatPanel->Refresh();
    book->SetSelection(static_cast<int>(page));
}

void MainFrame::TriggerFeedRefresh() {
    feedPanel->NextPage();
}

// ── Build Home Page ──────────────────────────────────────────────────────────
wxPanel* MainFrame::BuildHomePage(wxWindow* parent) {
    auto* page = new wxPanel(parent, wxID_ANY);

    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* topBar = new wxPanel(page, wxID_ANY);
    auto* hbox = new wxBoxSizer(wxHORIZONTAL);

    auto makeBtn = [&](wxPanel* parent, const wxString& label) {
        auto* btn = new wxButton(parent, wxID_ANY, label);
        UIButtons::ApplySizeBounds(btn, ButtonType::Nav);
        btn->SetBackgroundColour(UIColors::SurfaceRaised);
        btn->SetForegroundColour(UIColors::TextPrimary);
        return btn;
    };

    auto* settingsBtn = makeBtn(topBar, "Settings");
    auto* userBtn     = makeBtn(topBar, "User");
    auto* blendBtn    = makeBtn(topBar, "Blend");

    settingsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::SETTINGS); });
    userBtn    ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::USER); });
    blendBtn   ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){
        if (AppState::getInstance().getActiveBlend() != nullptr)
            NavigateTo(Page::BLEND_CHAT);
        else
            NavigateTo(Page::BLEND_CREATION);
    });

    hbox->Add(settingsBtn, 1, wxALL|wxEXPAND, 8);
    hbox->AddStretchSpacer(1);
    hbox->Add(userBtn, 1, wxALL|wxEXPAND, 8);
    hbox->Add(blendBtn, 1, wxALL|wxEXPAND, 8);
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
    auto* refreshBtn = new wxButton(refreshPanel, wxID_ANY, "Refresh");
    UIButtons::ApplySizeBounds(refreshBtn, ButtonType::Nav);
    const wxString refreshPath = ResolveResourcePath("refresh.png");
    if (!refreshPath.empty()) {
        wxBitmap refreshBmp(refreshPath, wxBITMAP_TYPE_PNG);
        if (refreshBmp.IsOk()) {
            refreshBmp = wxBitmap(refreshBmp.ConvertToImage().Rescale(18, 18));
            refreshBtn->SetBitmap(refreshBmp);
        }
    } else {
        wxLogWarning("Refresh icon not found; using text-only button.");
    }
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

bool MainFrame::LoadImage(int key, const wxString& path)
{
    wxImage image;
    if (!image.LoadFile(path, wxBITMAP_TYPE_ANY) || !image.IsOk()) {
        return false;
    }

    images[key] = image;
    return true;
}