/**
 * \file TopBar.hpp
 * \brief Reusable interior-page header bar.
 * \author Jasmine Kumar
 *
 * Displays a left-aligned back button and a centred title label for interior
 * pages. The back button triggers the provided NavigateFn callback with the
 * configured destination page.
 */

#pragma once
#include <wx/wx.h>
#include "UIPages.hpp"

/**
 * \class TopBar
 * \brief Reusable horizontal top bar for interior pages.
 */
class TopBar : public wxPanel {
public:
    /**
     * \brief Construct a TopBar.
     * \param parent Parent window that owns the bar.
     * \param title Title text shown in the center.
     * \param nav Navigation callback used for back navigation.
     * \param backDest Destination page when the back button is clicked.
     */
    TopBar(wxWindow* parent, const wxString& title, NavigateFn nav, Page backDest);

private:
    /**
     * \brief Handle clicks on the back button.
     * \param evt Button click event.
     */
    void OnBack(wxCommandEvent& evt);

    /** Navigation callback forwarded from the parent panel. */
    NavigateFn m_nav;
    /** Destination page used when the back button is activated. */
    Page       m_backDest;
};
