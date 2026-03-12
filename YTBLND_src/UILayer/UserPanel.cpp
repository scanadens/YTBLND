#include "UserPanel.h"

#include <fstream>
#include <sstream>
#include <list>

#include <wx/filedlg.h>
#include <wx/statline.h>

#include "UIColors.h"
#include "UIPages.h"
#include "TopBar.h"
#include "ConfirmationDialog.h"
#include "../AppLayer/AppController.h"
#include "../AppLayer/AppState.h"
#include "../AppLayer/EventRouter.h"
#include "../ModelLayer/Blend.h"
#include "../ModelLayer/User.h"
#include "../ModelLayer/Video.h"

// ── Construction ──────────────────────────────────────────────────────────────

UserPanel::UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background);

    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar ────────────────────────────────────────────────────────────────
    auto* topBar = new TopBar(this, "My Account", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // ── User info section ─────────────────────────────────────────────────────
    auto* infoPanel = new wxPanel(this, wxID_ANY);
    infoPanel->SetBackgroundColour(UIColors::Surface);
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);

    m_usernameLabel = new wxStaticText(infoPanel, wxID_ANY, "Username: —");
    m_usernameLabel->SetForegroundColour(UIColors::TextPrimary);
    wxFont uf = m_usernameLabel->GetFont();
    uf.SetPointSize(12);
    m_usernameLabel->SetFont(uf);

    m_emailLabel = new wxStaticText(infoPanel, wxID_ANY, "Email: —");
    m_emailLabel->SetForegroundColour(UIColors::TextSecondary);

    infoSizer->Add(m_usernameLabel, 0, wxLEFT | wxTOP, 16);
    infoSizer->Add(m_emailLabel,    0, wxLEFT | wxTOP | wxBOTTOM, 16);
    infoPanel->SetSizer(infoSizer);
    root->Add(infoPanel, 0, wxEXPAND);

    // ── Divider ───────────────────────────────────────────────────────────────
    auto* divider = new wxStaticLine(this, wxID_ANY);
    divider->SetBackgroundColour(UIColors::Separator);
    root->Add(divider, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);

    // ── Button helper ─────────────────────────────────────────────────────────
    auto makeBtn = [this](const wxString& label,
                          const wxColour& bg,
                          const wxColour& fg) -> wxButton* {
        auto* btn = new wxButton(this, wxID_ANY, label,
                                 wxDefaultPosition, wxSize(-1, 40));
        btn->SetBackgroundColour(bg);
        btn->SetForegroundColour(fg);
        return btn;
    };

    // ── Upload Watch History ──────────────────────────────────────────────────
    auto* uploadBtn = makeBtn("Upload Watch History (CSV)",
                              UIColors::SurfaceRaised, UIColors::TextPrimary);
    root->Add(uploadBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);

    // ── Save Blend ────────────────────────────────────────────────────────────
    auto* saveBtn = makeBtn("Save Blend",
                            UIColors::SurfaceRaised, UIColors::TextPrimary);
    root->Add(saveBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    // ── Load Blend ────────────────────────────────────────────────────────────
    auto* loadBtn = makeBtn("Load Blend",
                            UIColors::SurfaceRaised, UIColors::TextPrimary);
    root->Add(loadBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    // ── Spacer pushes logout to bottom ────────────────────────────────────────
    root->AddStretchSpacer(1);

    // ── Logout ────────────────────────────────────────────────────────────────
    auto* logoutBtn = makeBtn("Logout",
                              UIColors::Danger, UIColors::TextPrimary);
    root->Add(logoutBtn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    SetSizer(root);

    // ── Bindings ──────────────────────────────────────────────────────────────
    uploadBtn->Bind(wxEVT_BUTTON, &UserPanel::OnUploadCSV, this);
    saveBtn  ->Bind(wxEVT_BUTTON, &UserPanel::OnSaveBlend, this);
    loadBtn  ->Bind(wxEVT_BUTTON, &UserPanel::OnLoadBlend, this);
    logoutBtn->Bind(wxEVT_BUTTON, &UserPanel::OnLogout,    this);
    Bind(wxEVT_SHOW,              &UserPanel::OnShow,      this);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void UserPanel::RefreshUserInfo()
{
    User* user = AppState::getInstance().getCurrentUser();
    if (user) {
        m_usernameLabel->SetLabel("Username: " +
                                  wxString::FromUTF8(user->getUsername()));
        m_emailLabel->SetLabel("Email: " +
                               wxString::FromUTF8(user->getEmail()));
    } else {
        m_usernameLabel->SetLabel("Not logged in");
        m_emailLabel->SetLabel("");
    }
    Layout();
}

// ── Event handlers ────────────────────────────────────────────────────────────

void UserPanel::OnShow(wxShowEvent& evt)
{
    if (evt.IsShown())
        RefreshUserInfo();
    evt.Skip();
}

void UserPanel::OnUploadCSV(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(
        this,
        "Replace YouTube Data",
        "This will replace your current YouTube data. Continue?");
    if (!confirmed) return;

    wxFileDialog dlg(this, "Select Watch History CSV", "", "",
                     "CSV files (*.csv)|*.csv",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string path = dlg.GetPath().ToStdString();
    m_controller.getEventRouter().dispatch("uploadData", {{"filePath", path}});
}

void UserPanel::OnSaveBlend(wxCommandEvent& /*evt*/)
{
    Blend* blend = AppState::getInstance().getActiveBlend();
    if (!blend) {
        wxMessageBox("No active blend to save.", "Save Blend",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this, "Save Blend As", "", "blend.json",
                     "JSON files (*.json)|*.json",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string path = dlg.GetPath().ToStdString();

    // ── Build JSON manually ───────────────────────────────────────────────────
    std::ostringstream js;
    js << "{\n";
    js << "  \"blendID\": \"" << blend->getBlendID() << "\",\n";
    js << "  \"algorithmUsed\": \"" << blend->getAlgorithmUsed() << "\",\n";

    // Participants
    js << "  \"participants\": [";
    {
        const std::list<User>& participants = blend->getParticipants();
        bool first = true;
        for (const User& u : participants) {
            if (!first) js << ", ";
            first = false;
            js << "\"" << u.getUserID() << "\"";
        }
    }
    js << "],\n";

    // Videos
    js << "  \"videos\": [\n";
    {
        const std::list<Video>& videos = blend->getVideoList();
        bool first = true;
        for (const Video& v : videos) {
            if (!first) js << ",\n";
            first = false;
            js << "    {";
            js << "\"videoID\": \"" << v.getVideoID() << "\", ";
            js << "\"title\": \""   << v.getTitle()   << "\", ";
            js << "\"channelID\": \"" << v.getChannelID() << "\"";
            js << "}";
        }
        if (!videos.empty()) js << "\n";
    }
    js << "  ]\n";
    js << "}\n";

    std::ofstream file(path);
    if (!file) {
        wxMessageBox("Failed to open file for writing:\n" + path,
                     "Save Error", wxOK | wxICON_ERROR, this);
        return;
    }
    file << js.str();
    file.close();

    if (file.fail()) {
        wxMessageBox("Error writing file:\n" + path,
                     "Save Error", wxOK | wxICON_ERROR, this);
    } else {
        wxMessageBox("Blend saved successfully.", "Save Blend",
                     wxOK | wxICON_INFORMATION, this);
    }
}

void UserPanel::OnLoadBlend(wxCommandEvent& /*evt*/)
{
    wxFileDialog dlg(this, "Open Blend File", "", "",
                     "JSON files (*.json)|*.json",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string path = dlg.GetPath().ToStdString();

    // Dispatch to backend (not yet implemented — show informational message)
    m_controller.getEventRouter().dispatch("loadBlend", {{"filePath", path}});

    wxMessageBox("Blend loaded (not yet wired to backend).",
                 "Load Blend", wxOK | wxICON_INFORMATION, this);
}

void UserPanel::OnLogout(wxCommandEvent& /*evt*/)
{
    bool confirmed = ConfirmationDialog::Confirm(
        this, "Log Out", "Log out?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("logout", {});
    m_nav(Page::HOME);
}
