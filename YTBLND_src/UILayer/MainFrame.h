// ============================================================================
// MainFrame.h — Root application window
//
// MainFrame is the single top-level wxFrame.  It contains a wxSimplebook
// that acts as a full-window page switcher (no visible tabs).  Every panel
// is created once at startup and kept alive for the lifetime of the app;
// navigation just changes which page is visible.
//
// PAGE ORDER (must match the Page enum in UIPages.h exactly):
//   0  LOGIN            — LoginPanel
//   1  DATA_INSTRUCTIONS— DataInstructionsPanel
//   2  BLEND_CREATION   — BlendCreationPanel
//   3  HOME             — built by BuildHomePage() (contains BlendFeedPanel)
//   4  USER             — UserPanel
//   5  SETTINGS         — stub wxPanel (back button only)
//   6  BLEND_CHAT       — BlendChatPanel
//
// NAVIGATION PATTERN
//   NavigateTo(page) is the only entry point.  It calls Reset() / Reload()
//   on the target panel (if applicable) before flipping the page so panels
//   always receive fresh data on arrival.
//
// TODO: Consider loading a splash / loading screen before pages are built.
// TODO: Settings page is a stub; replace with a real SettingsPanel when
//       implemented.
// ============================================================================

#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "UIPages.h"

class AppController;
class LoginPanel;
class DataInstructionsPanel;
class BlendFeedPanel;
class UserPanel;
class BlendCreationPanel;
class BlendChatPanel;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(AppController& controller);

    // Switch to a page. Call this from sub-panels via the NavigateFn callback.
    void NavigateTo(Page page);

    // Called by the Refresh button to tell the feed to advance its page.
    void TriggerFeedRefresh();

private:
    AppController& controller;

    wxSimplebook*           book;
    LoginPanel*             loginPanel;
    DataInstructionsPanel*  dataInstrPanel;
    BlendCreationPanel*     creationPanel;
    BlendFeedPanel*         feedPanel;
    UserPanel*              userPanel;
    wxPanel*                settingsPanel;  // stub — back button only for now
    BlendChatPanel*         chatPanel;

    // Builds the home page (top bar + title + feed + refresh) and returns it.
    // BlendFeedPanel is constructed separately (with book as parent) so it can
    // be reparented to the home page panel here.
    wxPanel* BuildHomePage();

    // Top-bar button handlers on the home page
    void OnSettings(wxCommandEvent&);
    void OnUser    (wxCommandEvent&);
    void OnBlend   (wxCommandEvent&);   // goes to BLEND_CHAT or BLEND_CREATION
    void OnRefresh (wxCommandEvent&);   // advances the feed by one page
};
