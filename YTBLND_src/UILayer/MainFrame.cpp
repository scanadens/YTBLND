/**
 * \file MainFrame.cpp
 * \brief Logic for root application window containing the full-window page switcher.
 * \author Jasmine Kumar
 * \author Shamar Pennant
 * \author Xavier Lusetti
 *
 */
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
#include "ActiveBlendsPanel.hpp"
#include "UploadProgressDialog.hpp"
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
    controller(controller),
    refreshBtn(nullptr),
    refreshInProgress(false)
{
    // Resolve the path to the theme.txt containing the UI colour themes
    UIColors::LoadThemesFromFile(ResolveResourcePath("theme.txt"));

    // resolve the path to the background image and load it as a wxImage
    const wxString bgPath = ResolveResourcePath("checkered_wave_background.jpg");
    if (bgPath.empty() || !LoadImage(BG_MAIN, bgPath)) {
        wxLogWarning("Background image not found. Checked ../resources and resources.");
    }
    
    // create the outer container panel with the app background colour
    auto* bgPanel = new wxPanel(this);
    bgPanel->SetBackgroundColour(UIColors::Background());
    
    // placing our new background panel in the book
    book = new wxSimplebook(bgPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);

    NavigateFn nav = [this](Page p) { NavigateTo(p); };

    // --- Initialize pages and wrap each in a panel that paints the background image --

    // the login panel — paints its own background image
    loginPanel = new LoginPanel(book, controller, nav, images[BG_MAIN]);
    book->AddPage(loginPanel, "Login");

    auto* dataPageBg = new wxPanel(book);
    dataPageBg->SetBackgroundColour(UIColors::Background());
    dataInstrPanel = new DataInstructionsPanel(dataPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(dataInstrPanel, 1, wxEXPAND);
        dataPageBg->SetSizer(s);
    }
    book->AddPage(dataPageBg, "DataInstructions");

    auto* creationPageBg = new wxPanel(book);
    creationPageBg->SetBackgroundColour(UIColors::Background());
    creationPanel = new BlendCreationPanel(creationPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(creationPanel, 1, wxEXPAND);
        creationPageBg->SetSizer(s);
    }
    book->AddPage(creationPageBg, "BlendCreation");

    auto* homePageBg = new wxPanel(book);
    homePageBg->SetBackgroundColour(UIColors::Background());
    feedPanel = new BlendFeedPanel(homePageBg, controller);
    {
        auto* homePage = BuildHomePage(homePageBg);
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(homePage, 1, wxEXPAND);
        homePageBg->SetSizer(s);
    }
    book->AddPage(homePageBg, "Home");

    auto* userPageBg = new wxPanel(book);
    userPageBg->SetBackgroundColour(UIColors::Background());
    userPanel = new UserPanel(userPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(userPanel, 1, wxEXPAND);
        userPageBg->SetSizer(s);
    }
    book->AddPage(userPageBg, "User");

    auto* settingsPageBg = new wxPanel(book);
    settingsPageBg->SetBackgroundColour(UIColors::Background());
    settingsPanel = new SettingsPanel(settingsPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(settingsPanel, 1, wxEXPAND);
        settingsPageBg->SetSizer(s);
    }
    book->AddPage(settingsPageBg, "Settings");

    auto* chatPageBg = new wxPanel(book);
    chatPageBg->SetBackgroundColour(UIColors::Background());
    chatPanel = new BlendChatPanel(chatPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(chatPanel, 1, wxEXPAND);
        chatPageBg->SetSizer(s);
    }
    book->AddPage(chatPageBg, "BlendChat");

    auto* activeBlendsPageBg = new wxPanel(book);
    activeBlendsPageBg->SetBackgroundColour(UIColors::Background());
    activeBlendsPanel = new ActiveBlendsPanel(activeBlendsPageBg, controller, nav);
    {
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(activeBlendsPanel, 1, wxEXPAND);
        activeBlendsPageBg->SetSizer(s);
    }
    book->AddPage(activeBlendsPageBg, "ActiveBlends");

    // 6. Layout: Book expands inside BackgroundPanel
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->Add(book, 1, wxEXPAND);
    bgPanel->SetSizer(rootSizer);

    // Layout: BackgroundPanel expands inside the Frame
    auto* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(bgPanel, 1, wxEXPAND);
    SetSizer(frameSizer);

    // Theme change listener
        controller.getEventRouter().registerListener("theme_updated",
            [this, &controller](const EventPayload& /*payload*/) {
                UIColors::UpdateUI(this);
                ReloadThemedIcons();
                controller.getEventRouter().dispatch("theme_bg_login", {});
            });

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
    if (page == Page::ACTIVE_BLENDS)  activeBlendsPanel->Refresh();
    book->SetSelection(static_cast<int>(page));
}

void MainFrame::TriggerFeedRefresh() {
    if (refreshInProgress) {
        return;
    }

    refreshInProgress = true;
    if (refreshBtn != nullptr) {
        refreshBtn->Disable();
    }

    UploadProgressDialog progress(this, "Refreshing Feed");
    progress.ShowModal();
    progress.UpdateProgress(0.08, "Preparing refresh...");

    controller.setProgressReporter([&progress](double ratio, const std::string& message) {
        progress.UpdateProgress(ratio, wxString::FromUTF8(message));
    });

    // NextPage may either advance the local page offset immediately or trigger
    // controller-level refresh work when a new blend sample is required.
    progress.UpdateProgress(0.2, "Updating feed...");
    try {
        feedPanel->NextPage();
        progress.UpdateProgress(1.0, "Feed refreshed");
    } catch (...) {
        controller.clearProgressReporter();
        if (refreshBtn != nullptr) {
            refreshBtn->Enable();
        }
        refreshInProgress = false;
        throw;
    }

    controller.clearProgressReporter();
    progress.Destroy();
    if (refreshBtn != nullptr) {
        refreshBtn->Enable();
    }
    refreshInProgress = false;
}

// ── Icon helpers ─────────────────────────────────────────────────────────────

static wxBitmap LoadThemedIcon(const wxString& name, int size = 24) {
    wxString themeName = UIColors::Current ? UIColors::Current->Name : wxString("Dark Mode");
    wxString folder = themeName.BeforeFirst(' ').Lower();
    wxString path = ResolveResourcePath("icons/" + folder + "/" + name + ".png");
    if (!path.empty()) {
        wxImage img(path, wxBITMAP_TYPE_PNG);
        if (img.IsOk()) {
            img.Rescale(size, size, wxIMAGE_QUALITY_BILINEAR);
            return wxBitmap(img);
        }
    }
    return wxNullBitmap;
}

enum class IconBase { Surface, Background };

static wxButton* MakeIconButton(wxWindow* parent, const wxString& iconName,
                                 IconBase base, int iconSize = 24) {
    wxColour initBg = (base == IconBase::Surface) ? UIColors::Surface() : UIColors::Background();
    auto* btn = new wxButton(parent, wxID_ANY, wxEmptyString,
                              wxDefaultPosition, wxSize(iconSize + 16, iconSize + 12),
                              wxBORDER_NONE);
    btn->SetBackgroundColour(initBg);
    wxBitmap bmp = LoadThemedIcon(iconName, iconSize);
    if (bmp.IsOk()) btn->SetBitmap(bmp);

    // Look up the CURRENT palette color on each hover/leave so it stays
    // correct after theme switches
    btn->Bind(wxEVT_ENTER_WINDOW, [btn](wxMouseEvent& e) {
        btn->SetBackgroundColour(UIColors::SurfaceRaised());
        btn->Refresh(); e.Skip();
    });
    btn->Bind(wxEVT_LEAVE_WINDOW, [btn, base](wxMouseEvent& e) {
        btn->SetBackgroundColour(
            (base == IconBase::Surface) ? UIColors::Surface() : UIColors::Background());
        btn->Refresh(); e.Skip();
    });
    return btn;
}

// ── Build Home Page ──────────────────────────────────────────────────────────
wxPanel* MainFrame::BuildHomePage(wxWindow* parent) {
    auto* page = new wxPanel(parent, wxID_ANY);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // ── Row 1: Top ribbon (Surface bg, left-aligned: settings + user icons) ──
    auto* ribbon = new wxPanel(page, wxID_ANY);
    ribbon->SetBackgroundColour(UIColors::Surface());
    ribbon->SetMinSize(wxSize(-1, 32));
    auto* ribbonSizer = new wxBoxSizer(wxHORIZONTAL);

    settingsIconBtn = MakeIconButton(ribbon, "settings", IconBase::Surface);
    userIconBtn     = MakeIconButton(ribbon, "user",     IconBase::Surface);
    auto* settingsBtn = settingsIconBtn;
    auto* userBtn     = userIconBtn;

    settingsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::SETTINGS); });
    userBtn    ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::USER); });

    ribbonSizer->Add(settingsBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    ribbonSizer->Add(userBtn,     0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    ribbonSizer->AddStretchSpacer(1);
    ribbon->SetSizer(ribbonSizer);

    // ── Row 2: Title row (YTBLND centered-left, blends + chat icons right) ───
    auto* titlePanel = new wxPanel(page, wxID_ANY);
    auto* titleBox = new wxBoxSizer(wxHORIZONTAL);

    auto* titleLabel = new wxStaticText(titlePanel, wxID_ANY, "YTBLND");
    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(38);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);
    titleLabel->SetForegroundColour(UIColors::Accent());

    blendsIconBtn = MakeIconButton(titlePanel, "blends", IconBase::Background, 32);
    chatIconBtn   = MakeIconButton(titlePanel, "chat",   IconBase::Background, 32);
    auto* blendsBtn = blendsIconBtn;
    auto* chatBtn   = chatIconBtn;

    blendsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::ACTIVE_BLENDS); });
    chatBtn  ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){
        if (AppState::getInstance().getActiveBlend() != nullptr)
            NavigateTo(Page::BLEND_CHAT);
        else
            NavigateTo(Page::BLEND_CREATION);
    });

    titleBox->AddStretchSpacer(1);
    titleBox->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 24);
    titleBox->AddStretchSpacer(1);
    titleBox->Add(blendsBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    titleBox->Add(chatBtn,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    titlePanel->SetSizer(titleBox);

    // ── Feed grid (unchanged) ────────────────────────────────────────────────
    feedPanel->Reparent(page);

    // ── Refresh icon (centered, no text, no container) ───────────────────────
    auto* refreshPanel = new wxPanel(page, wxID_ANY);
    auto* rbox = new wxBoxSizer(wxHORIZONTAL);
    refreshBtn = MakeIconButton(refreshPanel, "refresh", IconBase::Background, 40);
    refreshBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ TriggerFeedRefresh(); });
    rbox->AddStretchSpacer(1);
    rbox->Add(refreshBtn, 0, wxALL, 8);
    rbox->AddStretchSpacer(1);
    refreshPanel->SetSizer(rbox);

    vbox->Add(ribbon, 0, wxEXPAND);
    vbox->Add(titlePanel, 0, wxEXPAND);
    vbox->Add(feedPanel, 1, wxEXPAND | wxALL, 16);
    vbox->Add(refreshPanel, 0, wxEXPAND);
    vbox->AddSpacer(16);
    page->SetSizer(vbox);

    return page;
}

void MainFrame::ReloadThemedIcons() {
    auto reload = [](wxButton* btn, const wxString& name, int size) {
        if (!btn) return;
        wxBitmap bmp = LoadThemedIcon(name, size);
        if (bmp.IsOk()) btn->SetBitmap(bmp);
        btn->Refresh();
    };
    reload(settingsIconBtn, "settings", 24);
    reload(userIconBtn,     "user",     24);
    reload(blendsIconBtn,   "blends",   32);
    reload(chatIconBtn,     "chat",     32);
    reload(refreshBtn,      "refresh",  40);
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