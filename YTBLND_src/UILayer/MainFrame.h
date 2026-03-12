#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include "UIPages.h"

class AppController;
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

    wxSimplebook*       book;
    BlendFeedPanel*     feedPanel;
    UserPanel*          userPanel;
    wxPanel*            settingsPanel;   // stub — back button only for now
    BlendCreationPanel* creationPanel;
    BlendChatPanel*     chatPanel;

    // Builds the home page (top bar + title + feed + refresh) and returns it.
    wxPanel* BuildHomePage();

    // Top-bar button handlers on the home page
    void OnSettings(wxCommandEvent&);
    void OnUser    (wxCommandEvent&);
    void OnBlend   (wxCommandEvent&);
    void OnRefresh (wxCommandEvent&);
};
