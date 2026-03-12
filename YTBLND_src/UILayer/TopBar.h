// ============================================================================
// TopBar.h — Reusable interior-page header bar
//
// Displays a "< Back" button on the left and a bold, centred page title.
// Used by every interior page (BlendCreationPanel, BlendChatPanel,
// UserPanel, DataInstructionsPanel) as a consistent navigation header.
//
// Usage:
//   auto* bar = new TopBar(this, "Page Title", m_nav, Page::HOME);
//   root->Add(bar, 0, wxEXPAND);
//
// The back button calls m_nav(backDest) when clicked, which triggers
// MainFrame::NavigateTo(backDest) via the NavigateFn callback.
//
// TODO: Add an optional right-side action button slot (e.g. an edit icon)
//       for pages that need a contextual action alongside the back button.
// ============================================================================

#pragma once
#include <wx/wx.h>
#include "UIPages.h"

// A reusable horizontal top bar for interior pages.
// Displays a "< Back" button on the left and a centred bold title.
class TopBar : public wxPanel {
public:
    // parent   — parent window (typically the panel that owns this bar)
    // title    — text displayed in the centre of the bar
    // nav      — NavigateFn callback from the parent panel
    // backDest — page to navigate to when "< Back" is pressed
    TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest);

private:
    void OnBack(wxCommandEvent& evt);

    NavigateFn m_nav;
    Page       m_backDest;
};
