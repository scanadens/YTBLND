#pragma once
#include <wx/wx.h>
#include "UIPages.h"

// A reusable horizontal top bar for interior pages.
// Displays a "< Back" button on the left and a centred bold title.
class TopBar : public wxPanel {
public:
    TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest);

private:
    void OnBack(wxCommandEvent& evt);

    NavigateFn m_nav;
    Page       m_backDest;
};
