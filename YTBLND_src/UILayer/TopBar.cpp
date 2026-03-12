// ============================================================================
// TopBar.cpp — Reusable interior-page header bar implementation
// ============================================================================

#include "TopBar.h"
#include "UIColors.h"
#include <wx/font.h>

TopBar::TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest)
    : wxPanel(parent, wxID_ANY)
    , m_nav(std::move(nav))
    , m_backDest(backDest)
{
    SetBackgroundColour(UIColors::Surface);

    // ── Back button ──────────────────────────────────────────────────────────
    wxButton* backBtn = new wxButton(this, wxID_ANY, wxT("< Back"),
                                     wxDefaultPosition, wxDefaultSize,
                                     wxBORDER_NONE);
    backBtn->SetBackgroundColour(UIColors::SurfaceRaised);
    backBtn->SetForegroundColour(UIColors::TextPrimary);
    backBtn->SetMinSize(wxSize(80, 30));

    backBtn->Bind(wxEVT_BUTTON, &TopBar::OnBack, this);

    // Lighten on hover for subtle feedback
    backBtn->Bind(wxEVT_ENTER_WINDOW, [backBtn](wxMouseEvent& e) {
        backBtn->SetBackgroundColour(wxColour(55, 55, 55));
        backBtn->Refresh();
        e.Skip();
    });
    backBtn->Bind(wxEVT_LEAVE_WINDOW, [backBtn](wxMouseEvent& e) {
        backBtn->SetBackgroundColour(UIColors::SurfaceRaised);
        backBtn->Refresh();
        e.Skip();
    });

    // ── Title label ──────────────────────────────────────────────────────────
    wxStaticText* titleLabel = new wxStaticText(this, wxID_ANY, title,
                                                wxDefaultPosition, wxDefaultSize,
                                                wxALIGN_CENTER_HORIZONTAL);
    titleLabel->SetForegroundColour(UIColors::TextPrimary);

    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);

    // ── Layout ───────────────────────────────────────────────────────────────
    // Three-column sizer: [back btn | title (stretch) | right spacer]
    // The right spacer mirrors the back button width so the title stays centred.
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    sizer->Add(backBtn,   0, wxALIGN_CENTER_VERTICAL | wxLEFT,  10);
    sizer->AddStretchSpacer(1);
    sizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->AddStretchSpacer(1);
    // Invisible right-side spacer equal to back-button width + left margin
    sizer->Add(80 + 10, 0);

    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 8);

    SetSizer(outerSizer);
    SetMinSize(wxSize(-1, 48));
}

void TopBar::OnBack(wxCommandEvent& /*evt*/)
{
    if (m_nav) {
        m_nav(m_backDest);
    }
}
