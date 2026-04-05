/**
 * \file MainFrame.hpp
 * \brief Root application window containing the full-window page switcher.
 * \author Jasmine Kumar
 * \author Shamar Pennant
 * \author Xavier Lusetti
 *
 * MainFrame is the single top-level wxFrame.  It contains a wxSimplebook that
 * acts as a full-window page switcher (no visible tabs).  Every panel is
 * created once at startup and kept alive for the application's lifetime;
 * navigation simply changes which page is visible.
 *
 * ### Page order (must match the Page enum in UIPages.hpp exactly)
 * | Index | Enum               | Panel                  |
 * |-------|--------------------|------------------------|
 * | 0     | LOGIN              | LoginPanel             |
 * | 1     | DATA_INSTRUCTIONS  | DataInstructionsPanel  |
 * | 2     | BLEND_CREATION     | BlendCreationPanel     |
 * | 3     | HOME               | BlendFeedPanel wrapper |
 * | 4     | USER               | UserPanel              |
 * | 5     | SETTINGS           | SettingsPanel          |
 * | 6     | BLEND_CHAT         | BlendChatPanel         |
 */

#pragma once
#include <unordered_map>
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "UIPages.hpp"
#include "UIColors.hpp"

class AppController;
class LoginPanel;
class DataInstructionsPanel;
class BlendFeedPanel;
class UserPanel;
class SettingsPanel;
class BlendCreationPanel;
class BlendChatPanel;
class ActiveBlendsPanel;

/**
 * \brief Root application window containing the full-window page switcher.
 */
/**
 * \class MainFrame
 * \brief MainFrame class declaration.
 */
class MainFrame : public wxFrame {
public:
    /**
     * Constructs and shows the MainFrame.
     * All sub-panels are created and added to the wxSimplebook here.
     * \param controller Application controller shared by all panels.
     */
    explicit MainFrame(AppController& controller);

    /**
        * Switches to the specified page and calls the target panel's Refresh()
        * method where applicable so it always shows fresh data on arrival.
     * \param page Page to navigate to.
     */
    void NavigateTo(Page page);

    /**
     * Advances the blend feed by one page of videos.
     * Called by the Refresh button on the home screen.
     */
    void TriggerFeedRefresh();

private:
    enum class IconBase {
        Surface,
        Background,
    };

    enum ImageKey {
        BG_MAIN = 0,
    };

    AppController& controller;

    wxSimplebook*          book;
    LoginPanel*            loginPanel;
    DataInstructionsPanel* dataInstrPanel;
    BlendCreationPanel*    creationPanel;
    BlendFeedPanel*        feedPanel;
    UserPanel*             userPanel;
    SettingsPanel*         settingsPanel;
    BlendChatPanel*        chatPanel;
    ActiveBlendsPanel*     activeBlendsPanel;
    // Home-page icon button references for theme-switch icon reloading.
    wxButton*              refreshBtn;
    wxButton*              settingsIconBtn = nullptr;
    wxButton*              userIconBtn     = nullptr;
    wxButton*              blendsIconBtn   = nullptr;
    wxButton*              chatIconBtn     = nullptr;
    wxStaticText*          blendIndicatorLabel = nullptr;
    // Guards against duplicate refresh dispatches from double-clicks.
    bool                   refreshInProgress;

    /// Reloads all themed icon bitmaps after a theme switch.
    void ReloadThemedIcons();
    /// Updates the home-page active blend title label from controller state.
    void UpdateBlendIndicatorLabel();

    /**
     * \brief Load a themed icon bitmap from the resources directory.
     * \param name Icon base name without extension.
     * \param size Target icon size in pixels.
     * \return Loaded bitmap or wxNullBitmap if unavailable.
     */
    static wxBitmap LoadThemedIcon(const wxString& name, int size = 24);

    /**
     * \brief Create a button that displays a themed icon and hover background.
     * \param parent Parent window that owns the button.
     * \param iconName Icon base name without extension.
     * \param base Determines which base background color to use.
     * \param iconSize Icon size in pixels.
     * \return Newly created icon button.
     */
    wxButton* MakeIconButton(wxWindow* parent, const wxString& iconName,
                             IconBase base, int iconSize = 24);

    /**
     * \brief Load and apply available application icon assets to this frame.
     */
    void ApplyAppIcons();

    std::unordered_map<int, wxImage> images; // images

    /**
     * Builds the home page composite (top bar + title + feed + refresh button).
     * \return Pointer to the constructed home page wxPanel.
     */
    wxPanel* BuildHomePage(wxWindow* parent);
    wxPanel* BuildSettingsPage(wxWindow* parent);

    void OnSettings(wxCommandEvent&);  ///< Navigates to SETTINGS.
    void OnUser    (wxCommandEvent&);  ///< Navigates to USER.
    void OnBlend   (wxCommandEvent&);  ///< Navigates to BLEND_CHAT or BLEND_CREATION.
    void OnRefresh (wxCommandEvent&);  ///< Advances the feed by one page.

    bool LoadImage(int key, const wxString& path);
};