/**
 * \file TopBar.cpp
 * \brief Implementation for TopBar.
 * \author Jasmine Kumar
 * \author Xavier Lusetti
 */

// ============================================================================
// TopBar.cpp - Reusable interior-page header bar implementation
// ============================================================================

#include "TopBar.hpp"
#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "ResourcePathUtils.hpp"
#include <wx/font.h>
#include <wx/image.h>

TopBar::TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest)
    : wxPanel(parent, wxID_ANY)
    , m_nav(std::move(nav))
    , m_backDest(backDest)
{
    SetBackgroundColour(UIColors::Background());

    // -- Back icon button (standalone on Background) --------------------------
    m_backBtn = new wxButton(this, wxID_ANY, wxEmptyString,
                              wxDefaultPosition, wxSize(32, 32),
                              wxBORDER_NONE);
    OnThemeUpdate();

    // Back button event bindings
    m_backBtn->Bind(wxEVT_BUTTON, &TopBar::OnBack, this);
        // Hover effects
    m_backBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) {
        m_backBtn->SetBackgroundColour(UIColors::SurfaceRaised());
        m_backBtn->Refresh(); 
        e.Skip();
    });
    m_backBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& e) {
        m_backBtn->SetBackgroundColour(UIColors::Background());
        m_backBtn->Refresh(); 
        e.Skip();
    });

    // -- Title label ----------------------------------------------------------
    wxStaticText* titleLabel = new wxStaticText(this, wxID_ANY, title,
                                                wxDefaultPosition, wxDefaultSize,
                                                wxALIGN_CENTER_HORIZONTAL);
    titleLabel->SetForegroundColour(UIColors::TextPrimary());

    wxFont titleFont = titleLabel->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(titleFont);

    // -- Layout ---------------------------------------------------------------
    // Three-column sizer: [back btn | title (stretch) | right spacer]
    // The right spacer mirrors the back button width so the title stays centred.
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(m_backBtn,    0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    sizer->AddStretchSpacer(1);
    sizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->AddStretchSpacer(1);
    sizer->Add(36 + 8, 0); // right spacer to balance the back icon

    wxBoxSizer* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(sizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 4);

    SetSizer(outerSizer);
    SetMinSize(wxSize(-1, 40));
}

void TopBar::OnThemeUpdate() {
    if (!m_backBtn) return;

    // Update the button background to match the new theme
    m_backBtn->SetBackgroundColour(UIColors::Background());

    // Fetch the updated SVG icon
    wxBitmapBundle icon = UIColors::GetIcon("back.svg", ColorRole::TextPrimary, 32);
    
    if (icon.IsOk()) {
        m_backBtn->SetBitmap(icon);
    }

    // Repaint
    m_backBtn->Refresh();
}

void TopBar::OnBack(wxCommandEvent& /*evt*/)
{
    if (m_nav) {
        m_nav(m_backDest);
    }
}
