/**
 * \file UserPanel.hpp
 * \brief Account info and logout panel.
 * \author Jasmine Kumar
 *
 * A panel that shows the current user's username and email, plus logout and
 * account-deletion controls.
 *
 * LAYOUT
 * ------
 * TopBar ("My Account", back -> HOME)
 * Info panel with username and email labels
 * Separator and spacer
 * Action buttons (logout and delete-account flow)
 *
 * REFRESH ON SHOW
 * ---------------
 * RefreshUserInfo() runs whenever the panel is shown (wxEVT_SHOW) so labels
 * always reflect current AppState.
 *
 * LOGOUT
 * ------
 * OnLogout: ConfirmationDialog -> dispatch("logout") -> nav(LOGIN).
 *
 * FUTURE FEATURES
 * ---------------
 * TODO: Add a "Change Password" form.
 * TODO: Show YouTube data status and a CSV re-upload action.
 * TODO: Display blend history.
 */

#pragma once

#include <wx/wx.h>

#include "IRefreshablePanel.hpp"
#include "UIPages.hpp"

class AppController;
class wxStaticText;
class wxTextCtrl;
class wxButton;

/**
 * \class UserPanel
 * \brief Panel for user account display and account actions.
 */
class UserPanel : public wxPanel, public IRefreshablePanel {
public:
    /**
     * \brief Construct a UserPanel.
     * \param parent Parent wxWidgets window.
     * \param controller Application controller reference.
     * \param nav Page navigation callback.
     */
    UserPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

    /**
     * \brief Refresh panel UI from current AppState.
     */
    void Refresh() override;

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    /** Labels that display the logged-in user's info. */
    wxStaticText* m_usernameLabel;
    wxStaticText* m_emailLabel;
    wxTextCtrl*   m_deletePasswordField;
    wxStaticText* m_deleteErrorLabel;
    wxButton*     m_confirmDeleteBtn;

    /** Refresh user labels from AppState. */
    void RefreshUserInfo();

    /** Show or hide delete-account re-auth controls. */
    void SetDeleteReauthVisible(bool isVisible);

    /** Handle initial delete-account request button click. */
    void OnDeleteRequest(wxCommandEvent& evt);
    /** Handle delete-account confirmation click. */
    void OnConfirmDelete(wxCommandEvent& evt);
    /** Handle CSV re-upload action. */
    void OnReuploadCSV(wxCommandEvent& evt);
    /** Handle logout action. */
    void OnLogout (wxCommandEvent& evt);
    /** Handle panel show event to trigger refresh. */
    void OnShow   (wxShowEvent&    evt);
};
