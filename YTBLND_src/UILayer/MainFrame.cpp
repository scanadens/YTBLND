/**
 * \file MainFrame.cpp
 * \brief Implementation for MainFrame.
 * \author Jasmine Kumar
 * \author Shamar Pennant
 * \author Xavier Lusetti
 */

#include "MainFrame.hpp"
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/dcbuffer.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "ResourcePathUtils.hpp"
#include "ConfirmationDialog.hpp"

#include <wx/popupwin.h>
#include <wx/filedlg.h>
#include <wx/statline.h>
#include "LoginPanel.hpp"
#include "DataInstructionsPanel.hpp"
#include "BlendFeedPanel.hpp"
#include "UserPanel.hpp"
#include "SettingsPanel.hpp"
#include "BlendCreationPanel.hpp"
#include "BlendChatPanel.hpp"
#include "ActiveBlendsPanel.hpp"
#include "OperationProgressDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"

void MainFrame::ApplyAppIcons()
{
    const wxArrayString iconCandidates = {
        "ytblnd-framev2.png",
        "ytblnd-starv2.png",
        "ytblnd-star.png",
    };

    wxIconBundle iconBundle;
    for (const auto& fileName : iconCandidates) {
        const wxString iconPath = UIResourcePaths::FindResourcePath(fileName);
        if (iconPath.empty()) {
            continue;
        }

        wxIcon icon;
        if (icon.LoadFile(iconPath, wxBITMAP_TYPE_PNG) && icon.IsOk()) {
            iconBundle.AddIcon(icon);
        }
    }

    if (!iconBundle.IsEmpty()) {
        SetIcons(iconBundle);
    } else {
        wxLogWarning("App icon files not found in resources.");
    }
}

// -- Background Panel ---------------------------------------------------------
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

// -- Construction -------------------------------------------------------------
MainFrame::MainFrame(AppController& controller)
    : wxFrame(nullptr, wxID_ANY, "YTBLND",
              wxDefaultPosition, wxSize(1280, 800),
              wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
    controller(controller),
    refreshBtn(nullptr),
    refreshInProgress(false)
{
    // Resolve the path to the theme.txt containing the UI colour themes
    UIColors::LoadThemesFromFile(UIResourcePaths::FindResourcePath("theme.txt"));

    ApplyAppIcons();

    // resolve the path to the background image and load it as a wxImage
    const wxString bgPath = UIResourcePaths::FindResourcePath("checkered_wave_background.jpg");
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

    // the login panel - paints its own background image
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

// -- Navigation ----------------------------------------------------------------
void MainFrame::NavigateTo(Page page) {
    if (page == Page::LOGIN)           loginPanel->Refresh();
    if (page == Page::BLEND_CREATION)  creationPanel->Refresh();
    if (page == Page::HOME) {
        feedPanel->Refresh();
        UpdateBlendIndicatorLabel();
    }
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

    OperationProgressDialog progress(this, "Refreshing Feed");
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

void MainFrame::UpdateBlendIndicatorLabel() {
    if (blendIndicatorLabel == nullptr) {
        return;
    }

    const std::string title = controller.get_current_blend_title();
    const wxString uiText = title.empty()
        ? wxString("No active blend selected")
        : wxString::FromUTF8(title);

    blendIndicatorLabel->SetLabel(uiText);
    blendIndicatorLabel->SetForegroundColour(UIColors::Accent());
    blendIndicatorLabel->Wrap(-1);
    blendIndicatorLabel->Refresh();
}

// -- Icon helpers -------------------------------------------------------------

wxBitmap MainFrame::LoadThemedIcon(const wxString& name, int size) {
    wxString folder = UIResourcePaths::GetIconFolder();
    wxString path = UIResourcePaths::FindResourcePath("icons/" + folder + "/" + name + ".png");
    if (!path.empty()) {
        wxImage img(path, wxBITMAP_TYPE_PNG);
        if (img.IsOk()) {
            img.Rescale(size, size, wxIMAGE_QUALITY_BILINEAR);
            return wxBitmap(img);
        }
    }
    return wxNullBitmap;
}

wxButton* MainFrame::MakeIconButton(wxWindow* parent, const wxString& iconName,
                                    IconBase base, int iconSize) {
    wxColour initBg = (base == IconBase::Surface) ? UIColors::Surface() : UIColors::Background();
    auto* btn = new wxButton(parent, wxID_ANY, wxEmptyString,
                              wxDefaultPosition, wxSize(iconSize + 16, iconSize + 12),
                              wxBORDER_NONE);
    btn->SetBackgroundColour(initBg);
    wxBitmap bmp = LoadThemedIcon(iconName, iconSize);
    if (bmp.IsOk()) btn->SetBitmap(bmp);

    // Capitalize first letter for display
    wxString tipText = iconName;
    if (!tipText.empty())
        tipText[0] = wxToupper(tipText[0]);

    // Shared tooltip popup pointer (one per button, created on first hover)
    auto* tipPtr = new wxPopupWindow*{nullptr};

    btn->Bind(wxEVT_ENTER_WINDOW, [btn, tipText, tipPtr](wxMouseEvent& e) {
        btn->SetBackgroundColour(UIColors::SurfaceRaised());
        btn->Refresh();

        // Create themed tooltip near the cursor
        if (!*tipPtr) {
            auto* tip = new wxPopupWindow(btn->GetParent());
            auto* tipPanel = new wxPanel(tip, wxID_ANY);
            tipPanel->SetBackgroundColour(UIColors::SurfaceRaised());
            auto* lbl = new wxStaticText(tipPanel, wxID_ANY, tipText);
            lbl->SetForegroundColour(UIColors::TextMuted());
            lbl->SetBackgroundColour(UIColors::SurfaceRaised());
            auto* sz = new wxBoxSizer(wxHORIZONTAL);
            sz->Add(lbl, 0, wxALL, 6);
            tipPanel->SetSizer(sz);
            tipPanel->Fit();
            tip->SetClientSize(tipPanel->GetBestSize());
            *tipPtr = tip;
        }
        // Position centered below the button
        wxPoint btnScreen = btn->ClientToScreen(wxPoint(0, btn->GetSize().GetHeight()));
        int tipW = (*tipPtr)->GetSize().GetWidth();
        int btnW = btn->GetSize().GetWidth();
        (*tipPtr)->SetPosition(wxPoint(btnScreen.x + (btnW - tipW) / 2, btnScreen.y + 4));
        (*tipPtr)->Show();
        e.Skip();
    });
    btn->Bind(wxEVT_LEAVE_WINDOW, [btn, base, tipPtr](wxMouseEvent& e) {
        btn->SetBackgroundColour(
            (base == IconBase::Surface) ? UIColors::Surface() : UIColors::Background());
        btn->Refresh();
        if (*tipPtr) {
            (*tipPtr)->Hide();
            (*tipPtr)->Destroy();
            *tipPtr = nullptr;
        }
        e.Skip();
    });
    return btn;
}

// -- Build Home Page ----------------------------------------------------------
wxPanel* MainFrame::BuildHomePage(wxWindow* parent) {
    auto* page = new wxPanel(parent, wxID_ANY);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // -- Row 1: Top ribbon (Surface bg, left-aligned: settings + user icons) --
    auto* ribbon = new wxPanel(page, wxID_ANY);
    ribbon->SetBackgroundColour(UIColors::Surface());
    ribbon->SetMinSize(wxSize(-1, 32));
    auto* ribbonSizer = new wxBoxSizer(wxHORIZONTAL);

    settingsIconBtn = MakeIconButton(ribbon, "settings", IconBase::Surface);
    userIconBtn     = MakeIconButton(ribbon, "user",     IconBase::Surface);
    auto* settingsBtn = settingsIconBtn;
    auto* userBtn     = userIconBtn;

    settingsBtn->Bind(wxEVT_BUTTON, [this, settingsBtn](wxCommandEvent&){ ShowSettingsDropdown(settingsBtn); });
    userBtn    ->Bind(wxEVT_BUTTON, [this, userBtn](wxCommandEvent&){ ShowUserDropdown(userBtn); });

    ribbonSizer->Add(settingsBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    ribbonSizer->Add(userBtn,     0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    ribbonSizer->AddStretchSpacer(1);
    ribbon->SetSizer(ribbonSizer);

    // -- Row 2: Title row (YTBLND centered-left, blends + chat icons right) ---
    auto* titlePanel = new wxPanel(page, wxID_ANY);
    auto* titleGrid = new wxGridSizer(1, 3, 0, 0);

    auto* titleLabel = new wxStaticText(titlePanel, wxID_ANY, "YTBLND");
    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(38);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);
    titleLabel->SetForegroundColour(UIColors::Accent());

    auto* leftSlot = new wxPanel(titlePanel, wxID_ANY);
    leftSlot->SetBackgroundColour(UIColors::Background());

    auto* rightSlot = new wxPanel(titlePanel, wxID_ANY);
    rightSlot->SetBackgroundColour(UIColors::Background());

    blendsIconBtn = MakeIconButton(rightSlot, "blends", IconBase::Background, 32);
    chatIconBtn   = MakeIconButton(rightSlot, "chat",   IconBase::Background, 32);
    auto* blendsBtn = blendsIconBtn;
    auto* chatBtn   = chatIconBtn;

    blendsBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ NavigateTo(Page::ACTIVE_BLENDS); });
    chatBtn  ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){
        if (AppState::getInstance().getActiveBlend() != nullptr)
            NavigateTo(Page::BLEND_CHAT);
        else
            NavigateTo(Page::BLEND_CREATION);
    });

    // Keep actions in the right third and right-align within that column.
    auto* rightControls = new wxBoxSizer(wxHORIZONTAL);
    rightControls->AddStretchSpacer(1);
    rightControls->Add(blendsBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    rightControls->Add(chatBtn,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    rightSlot->SetSizer(rightControls);

    // Equal-width 3-column grid: empty left slot, centered title, right actions.
    // This keeps the title centered in the full frame while scaling naturally.
    titleGrid->Add(leftSlot, 1, wxEXPAND);
    titleGrid->Add(titleLabel, 0, wxALIGN_CENTER);
    titleGrid->Add(rightSlot, 1, wxEXPAND);
    titlePanel->SetSizer(titleGrid);

    // -- Feed grid (unchanged) ------------------------------------------------
    feedPanel->Reparent(page);

    // -- Refresh icon (centered, no text, no container) -----------------------
    auto* refreshPanel = new wxPanel(page, wxID_ANY);
    auto* rbox = new wxBoxSizer(wxHORIZONTAL);
    refreshBtn = MakeIconButton(refreshPanel, "refresh", IconBase::Background, 40);
    refreshBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ TriggerFeedRefresh(); });
    rbox->AddStretchSpacer(1);
    rbox->Add(refreshBtn, 0, wxALL, 8);
    rbox->AddStretchSpacer(1);
    refreshPanel->SetSizer(rbox);

    // setting up the label to indicate the current blend
    wxPanel* bi_panel = new wxPanel(page, wxID_ANY);

    bi_panel->SetBackgroundColour(UIColors::Background());
    blendIndicatorLabel = new wxStaticText(bi_panel, wxID_ANY, wxEmptyString);
    wxFont indicator_font = blendIndicatorLabel->GetFont();
    indicator_font.SetPointSize(16);
    indicator_font.SetWeight(wxFONTWEIGHT_LIGHT);
    blendIndicatorLabel->SetFont(indicator_font);
    blendIndicatorLabel->SetForegroundColour(UIColors::TextPrimary());
    UpdateBlendIndicatorLabel();

    auto* BI_lbl_box = new wxBoxSizer(wxHORIZONTAL);
    BI_lbl_box->AddStretchSpacer(1);
    BI_lbl_box->Add(blendIndicatorLabel);
    BI_lbl_box->AddStretchSpacer(1);
    bi_panel->SetSizer(BI_lbl_box);

    vbox->Add(ribbon, 0, wxEXPAND);
    vbox->Add(titlePanel, 0, wxEXPAND);
    vbox->Add(bi_panel, 0, wxEXPAND | wxCENTER);
    vbox->Add(feedPanel, 1, wxEXPAND | wxALL, 16);
    vbox->Add(refreshPanel, 0, wxEXPAND);
    vbox->AddSpacer(16);
    page->SetSizer(vbox);

    return page;
}

void MainFrame::ReloadThemedIcons() {
    auto reload = [](wxButton* btn, const wxString& name, int size) {
        if (!btn) return;
        wxBitmap bmp = MainFrame::LoadThemedIcon(name, size);
        if (bmp.IsOk()) btn->SetBitmap(bmp);
        btn->Refresh();
    };
    reload(settingsIconBtn, "settings", 24);
    reload(userIconBtn,     "user",     24);
    reload(blendsIconBtn,   "blends",   32);
    reload(chatIconBtn,     "chat",     32);
    reload(refreshBtn,      "refresh",  40);
}

// -- Helper: create a hoverable row for dropdown menus -------------------------
namespace {
wxPanel* MakeDropdownRow(wxWindow* parent, const wxString& label,
                          const wxColour& textColor = UIColors::TextMuted()) {
    auto* row = new wxPanel(parent, wxID_ANY);
    row->SetBackgroundColour(UIColors::Surface());
    row->SetMinSize(wxSize(220, 40));
    auto* sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* lbl = new wxStaticText(row, wxID_ANY, label);
    lbl->SetForegroundColour(textColor);
    lbl->SetBackgroundColour(UIColors::Surface());
    sizer->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 16);
    row->SetSizer(sizer);

    for (wxWindow* w : {static_cast<wxWindow*>(row), static_cast<wxWindow*>(lbl)}) {
        w->Bind(wxEVT_ENTER_WINDOW, [row, lbl](wxMouseEvent& e) {
            row->SetBackgroundColour(UIColors::SurfaceRaised());
            lbl->SetBackgroundColour(UIColors::SurfaceRaised());
            row->Refresh(); e.Skip();
        });
        w->Bind(wxEVT_LEAVE_WINDOW, [row, lbl](wxMouseEvent& e) {
            row->SetBackgroundColour(UIColors::Surface());
            lbl->SetBackgroundColour(UIColors::Surface());
            row->Refresh(); e.Skip();
        });
    }
    return row;
}
}

// -- Settings dropdown ---------------------------------------------------------

void MainFrame::ShowSettingsDropdown(wxWindow* anchor) {
    auto* popup = new wxPopupTransientWindow(this);
    auto* panel = new wxPanel(popup, wxID_ANY);
    panel->SetBackgroundColour(UIColors::Surface());
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // "Theme >" row — toggles the theme list below
    auto* themeRow = MakeDropdownRow(panel, "Theme  >");
    sizer->Add(themeRow, 0, wxEXPAND);

    // Theme sub-items (initially hidden)
    auto* themeSub = new wxPanel(panel, wxID_ANY);
    themeSub->SetBackgroundColour(UIColors::Surface());
    auto* themeSubSizer = new wxBoxSizer(wxVERTICAL);

    for (const wxString& name : UIColors::ThemeOrder) {
        auto* row = new wxPanel(themeSub, wxID_ANY);
        row->SetBackgroundColour(UIColors::Surface());
        row->SetMinSize(wxSize(220, 36));
        auto* rowSz = new wxBoxSizer(wxHORIZONTAL);
        auto* lbl = new wxStaticText(row, wxID_ANY, "   " + name);
        lbl->SetForegroundColour(UIColors::TextMuted());
        lbl->SetBackgroundColour(UIColors::Surface());
        rowSz->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 16);
        row->SetSizer(rowSz);

        // Hover
        for (wxWindow* w : {static_cast<wxWindow*>(row), static_cast<wxWindow*>(lbl)}) {
            w->Bind(wxEVT_ENTER_WINDOW, [row, lbl](wxMouseEvent& e) {
                row->SetBackgroundColour(UIColors::SurfaceRaised());
                lbl->SetBackgroundColour(UIColors::SurfaceRaised());
                row->Refresh(); e.Skip();
            });
            w->Bind(wxEVT_LEAVE_WINDOW, [row, lbl](wxMouseEvent& e) {
                row->SetBackgroundColour(UIColors::Surface());
                lbl->SetBackgroundColour(UIColors::Surface());
                row->Refresh(); e.Skip();
            });
        }

        // Click — apply theme
        wxString capName = name;
        auto themeAction = [this, popup, capName](wxMouseEvent&) {
            UIColors::SetTheme(capName);
            wxWindow* top = wxGetTopLevelParent(this);
            UIColors::UpdateUI(top);
            controller.getEventRouter().dispatch("theme_updated", {});
            popup->Dismiss();
        };
        row->Bind(wxEVT_LEFT_UP, themeAction);
        lbl->Bind(wxEVT_LEFT_UP, themeAction);

        themeSubSizer->Add(row, 0, wxEXPAND);
    }
    themeSub->SetSizer(themeSubSizer);
    themeSub->Hide();
    sizer->Add(themeSub, 0, wxEXPAND);

    // Toggle theme sub-list on click of "Theme >" row
    auto toggleThemes = [themeSub, panel, popup](wxMouseEvent&) {
        themeSub->Show(!themeSub->IsShown());
        panel->Layout();
        panel->Fit();
        popup->SetClientSize(panel->GetBestSize());
        popup->Layout();
    };
    themeRow->Bind(wxEVT_LEFT_UP, toggleThemes);
    for (wxWindow* w : themeRow->GetChildren())
        w->Bind(wxEVT_LEFT_UP, toggleThemes);

    panel->SetSizer(sizer);
    panel->Layout();
    panel->Fit();
    popup->SetClientSize(panel->GetBestSize());
    popup->Layout();

    wxPoint pos = anchor->ClientToScreen(wxPoint(0, anchor->GetSize().GetHeight()));
    popup->SetPosition(pos);
    popup->Popup();
}

// -- User dropdown -------------------------------------------------------------

void MainFrame::ShowUserDropdown(wxWindow* anchor) {
    auto* popup = new wxPopupTransientWindow(this);
    auto* panel = new wxPanel(popup, wxID_ANY);
    panel->SetBackgroundColour(UIColors::Surface());
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // Username display (centered, non-clickable)
    User* user = AppState::getInstance().getCurrentUser();
    wxString username = user ? wxString::FromUTF8(user->getUsername()) : "Not logged in";
    auto* nameLabel = new wxStaticText(panel, wxID_ANY, username,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxALIGN_CENTRE_HORIZONTAL);
    nameLabel->SetForegroundColour(UIColors::TextPrimary());
    nameLabel->SetBackgroundColour(UIColors::Surface());
    nameLabel->SetMinSize(wxSize(220, -1));
    wxFont nf = nameLabel->GetFont();
    nf.SetWeight(wxFONTWEIGHT_BOLD);
    nameLabel->SetFont(nf);
    sizer->Add(nameLabel, 0, wxEXPAND | wxALL, 10);

    // "Re-upload Data" row
    auto* reuploadRow = MakeDropdownRow(panel, "Re-upload Data");
    sizer->Add(reuploadRow, 0, wxEXPAND);

    auto reuploadAction = [this, popup](wxMouseEvent&) {
        popup->Dismiss();
        User* u = AppState::getInstance().getCurrentUser();
        if (!u) return;
        wxFileDialog dlg(this, "Select Watch Later CSV", "", "",
                         "Supported files (*.csv;*.html;*.htm)|*.csv;*.html;*.htm|All files (*.*)|*.*",
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() != wxID_OK) return;
        controller.getEventRouter().dispatch("uploadData",
            {{"filePath", dlg.GetPath().ToStdString()}, {"userID", u->getUserID()}});
        wxMessageBox("Your data has been re-uploaded.", "Data Updated", wxOK | wxICON_INFORMATION, this);
    };
    reuploadRow->Bind(wxEVT_LEFT_UP, reuploadAction);
    for (wxWindow* w : reuploadRow->GetChildren())
        w->Bind(wxEVT_LEFT_UP, reuploadAction);

    // "Delete Account" row (same muted color as others, no red)
    auto* deleteRow = MakeDropdownRow(panel, "Delete Account");
    sizer->Add(deleteRow, 0, wxEXPAND);

    auto deleteAction = [this, popup](wxMouseEvent&) {
        popup->Dismiss();
        User* u = AppState::getInstance().getCurrentUser();
        if (!u) return;
        bool confirmed = ConfirmationDialog::Confirm(this, "Delete Account",
            "Are you sure you want to delete your account? This cannot be undone.");
        if (!confirmed) return;
        wxTextEntryDialog pwDlg(this, "Re-enter your password:", "Confirm Delete");
        if (pwDlg.ShowModal() != wxID_OK) return;
        std::string password = pwDlg.GetValue().ToStdString();
        if (password.empty()) return;
        controller.getEventRouter().dispatch("deleteAccount",
            {{"userID", u->getUserID()}, {"password", password}});
        if (AppState::getInstance().getCurrentUser() == nullptr) {
            wxMessageBox("Account deleted.", "Done", wxOK | wxICON_INFORMATION, this);
            NavigateTo(Page::LOGIN);
        } else {
            wxMessageBox("Delete failed. Check password.", "Error", wxOK | wxICON_ERROR, this);
        }
    };
    deleteRow->Bind(wxEVT_LEFT_UP, deleteAction);
    for (wxWindow* w : deleteRow->GetChildren())
        w->Bind(wxEVT_LEFT_UP, deleteAction);

    // "Log Out" row
    auto* logoutRow = MakeDropdownRow(panel, "Log Out");
    sizer->Add(logoutRow, 0, wxEXPAND);

    auto logoutAction = [this, popup](wxMouseEvent&) {
        popup->Dismiss();
        bool confirmed = ConfirmationDialog::Confirm(this, "Log Out", "Log out?");
        if (!confirmed) return;
        controller.getEventRouter().dispatch("logout", {});
        NavigateTo(Page::LOGIN);
    };
    logoutRow->Bind(wxEVT_LEFT_UP, logoutAction);
    for (wxWindow* w : logoutRow->GetChildren())
        w->Bind(wxEVT_LEFT_UP, logoutAction);

    panel->SetSizer(sizer);
    panel->Layout();
    panel->Fit();
    popup->SetClientSize(panel->GetBestSize());
    popup->Layout();

    wxPoint pos = anchor->ClientToScreen(wxPoint(0, anchor->GetSize().GetHeight()));
    popup->SetPosition(pos);
    popup->Popup();
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