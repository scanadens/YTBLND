// ============================================================================
// DataInstructionsPanel.cpp — Google Takeout CSV setup screen implementation
// ============================================================================

#include "DataInstructionsPanel.hpp"

#include <wx/filedlg.h>
#include <wx/statline.h>

#include "UIColors.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/User.hpp"

DataInstructionsPanel::DataInstructionsPanel(wxWindow*      parent,
                                             AppController& controller,
                                             NavigateFn     nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background);

    auto* outer = new wxBoxSizer(wxVERTICAL);

    // ── Heading ───────────────────────────────────────────────────────────────
    auto* heading = new wxStaticText(this, wxID_ANY,
                                     "Set Up Your YouTube Data",
                                     wxDefaultPosition, wxDefaultSize,
                                     wxALIGN_CENTRE_HORIZONTAL);
    {
        wxFont f = heading->GetFont();
        f.SetPointSize(22); f.SetWeight(wxFONTWEIGHT_BOLD);
        heading->SetFont(f);
    }
    heading->SetForegroundColour(UIColors::TextPrimary);

    auto* sub = new wxStaticText(this, wxID_ANY,
        "YTBLND needs your YouTube Watch Later playlist to create a blend.",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sub->SetForegroundColour(UIColors::TextSecondary);

    outer->AddStretchSpacer(1);
    outer->Add(heading, 0, wxALIGN_CENTER | wxBOTTOM, 8);
    outer->Add(sub,     0, wxALIGN_CENTER | wxBOTTOM, 32);

    // ── Steps card ────────────────────────────────────────────────────────────
    auto* card = new wxPanel(this, wxID_ANY);
    card->SetBackgroundColour(UIColors::Surface);
    card->SetMinSize(wxSize(520, -1));
    auto* cardSizer = new wxBoxSizer(wxVERTICAL);

    const wxString steps =
        "1.  Go to  takeout.google.com\n"
        "2.  Click \"Deselect all\", then tick  YouTube and YouTube Music\n"
        "3.  Under YouTube, select only  \"Watch later\"\n"
        "4.  Export and download the archive\n"
        "5.  Inside the zip, find:\n"
        "       Takeout / YouTube and YouTube Music / playlists /\n"
        "       Watch later videos.csv\n"
        "6.  Click \"Browse for CSV...\" below and select that file";

    auto* stepsLabel = new wxStaticText(card, wxID_ANY, steps);
    stepsLabel->SetForegroundColour(UIColors::TextPrimary);
    {
        wxFont f = stepsLabel->GetFont(); f.SetPointSize(11);
        stepsLabel->SetFont(f);
    }

    auto* div = new wxStaticLine(card, wxID_ANY);
    div->SetBackgroundColour(UIColors::Separator);

    auto* browseBtn = new wxButton(card, wxID_ANY, "Browse for CSV...",
                                   wxDefaultPosition, wxSize(-1, 40));
    browseBtn->SetBackgroundColour(UIColors::Accent);
    browseBtn->SetForegroundColour(UIColors::TextPrimary);

    auto* skipBtn = new wxButton(card, wxID_ANY, "Skip for now",
                                  wxDefaultPosition, wxSize(-1, 32));
    skipBtn->SetBackgroundColour(UIColors::SurfaceRaised);
    skipBtn->SetForegroundColour(UIColors::TextMuted);

    cardSizer->Add(stepsLabel, 0, wxALL, 24);
    cardSizer->Add(div,        0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    cardSizer->Add(browseBtn,  0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 20);
    cardSizer->Add(skipBtn,    0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    card->SetSizer(cardSizer);

    auto* hCentre = new wxBoxSizer(wxHORIZONTAL);
    hCentre->AddStretchSpacer(1);
    hCentre->Add(card, 0, wxEXPAND);
    hCentre->AddStretchSpacer(1);

    outer->Add(hCentre, 0, wxEXPAND);
    outer->AddStretchSpacer(2);
    SetSizer(outer);

    browseBtn->Bind(wxEVT_BUTTON, &DataInstructionsPanel::OnBrowse, this);
    skipBtn  ->Bind(wxEVT_BUTTON, &DataInstructionsPanel::OnSkip,   this);
}

void DataInstructionsPanel::OnBrowse(wxCommandEvent& /*evt*/)
{
    wxFileDialog dlg(this, "Select Watch Later CSV", "", "",
                     "CSV files (*.csv)|*.csv",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string path   = dlg.GetPath().ToStdString();
    User* user = AppState::getInstance().getCurrentUser();
    if (!user) return;

    m_controller.getEventRouter().dispatch("uploadData",
        {{"filePath", path}, {"userID", user->getUserID()}});

    m_nav(Page::BLEND_CREATION);
}

void DataInstructionsPanel::OnSkip(wxCommandEvent& /*evt*/)
{
    m_nav(Page::BLEND_CREATION);
}
