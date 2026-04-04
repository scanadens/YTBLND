// ============================================================================
// TopBar.cpp — Reusable interior-page header bar implementation
// ============================================================================

#include "TopBar.hpp"
#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include <wx/font.h>
#include <wx/filename.h>
#include <wx/image.h>

namespace {
wxString FindResourcePath(const wxString& fileName) {
    for (const auto& c : wxArrayString{
             "YTBLND_src/resources/" + fileName,
             "resources/" + fileName,
             "../resources/" + fileName}) {
        if (wxFileExists(c)) return c;
    }
    return wxEmptyString;
}

wxBitmap LoadBackIcon(int size = 24) {
    wxString themeName = UIColors::Current ? UIColors::Current->Name : wxString("Dark Mode");
    wxString folder = themeName.BeforeFirst(' ').Lower();
    wxString path = FindResourcePath("icons/" + folder + "/back.png");
    if (!path.empty()) {
        wxImage img(path, wxBITMAP_TYPE_PNG);
        if (img.IsOk()) {
            img.Rescale(size, size, wxIMAGE_QUALITY_BILINEAR);
            return wxBitmap(img);
        }
    }
    return wxNullBitmap;
}
}

TopBar::TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest)
    : wxPanel(parent, wxID_ANY)
    , m_nav(std::move(nav))
    , m_backDest(backDest)
{
    SetBackgroundColour(UIColors::Background());

    // ── Back icon button (standalone on Background) ──────────────────────────
    auto* backBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxSize(36, 32),
                                  wxBORDER_NONE);
    backBtn->SetBackgroundColour(UIColors::Background());
    wxBitmap backBmp = LoadBackIcon(24);
    if (backBmp.IsOk()) backBtn->SetBitmap(backBmp);
    else backBtn->SetLabel("< Back");

    backBtn->Bind(wxEVT_BUTTON, &TopBar::OnBack, this);
    backBtn->Bind(wxEVT_ENTER_WINDOW, [backBtn](wxMouseEvent& e) {
        backBtn->SetBackgroundColour(UIColors::SurfaceRaised());
        backBtn->Refresh(); e.Skip();
    });
    backBtn->Bind(wxEVT_LEAVE_WINDOW, [backBtn](wxMouseEvent& e) {
        backBtn->SetBackgroundColour(UIColors::Background());
        backBtn->Refresh(); e.Skip();
    });

    // ── Title label ──────────────────────────────────────────────────────────
    wxStaticText* titleLabel = new wxStaticText(this, wxID_ANY, title,
                                                wxDefaultPosition, wxDefaultSize,
                                                wxALIGN_CENTER_HORIZONTAL);
    titleLabel->SetForegroundColour(UIColors::TextPrimary());

    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);

    // ── Layout: [back icon | stretch | title | stretch | spacer] ─────────────
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(backBtn,    0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddStretchSpacer(1);
    sizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->AddStretchSpacer(1);
    sizer->Add(36 + 8, 0); // right spacer to balance the back icon

    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 4);

    SetSizer(outerSizer);
    SetMinSize(wxSize(-1, 40));
}

void TopBar::OnBack(wxCommandEvent& /*evt*/)
{
    if (m_nav) {
        m_nav(m_backDest);
    }
}
