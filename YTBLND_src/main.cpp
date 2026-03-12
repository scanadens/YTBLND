// ============================================================================
// main.cpp — Application entry point
//
// Creates the single AppController (which owns AppState, EventRouter, and
// SqliteDataManager), initialises wxWidgets image handlers for thumbnail
// loading, then shows the MainFrame starting at the LOGIN page.
//
// Design note: AppController is a member of YTBLNDApp so its lifetime covers
// the entire run of the app.  MainFrame receives a reference and forwards it
// to every sub-panel via its constructor.
// ============================================================================

#include <wx/wx.h>
#include <wx/image.h>

#include "UILayer/MainFrame.h"
#include "AppLayer/AppController.h"

class YTBLNDApp : public wxApp {
public:
    bool OnInit() override;

private:
    // AppController owns AppState (singleton), EventRouter, and SqliteDataManager.
    // Declared here so its lifetime matches the application.
    AppController controller;
};

wxIMPLEMENT_APP(YTBLNDApp);

bool YTBLNDApp::OnInit() {
    // Enable JPEG and PNG handlers for VideoCard thumbnail loading.
    // Must be called before any wxImage::LoadFile() / wxURL usage.
    wxInitAllImageHandlers();

    // MainFrame starts maximised and shows the LOGIN page.
    // TODO: restore last window position/size from user preferences if desired.
    auto* frame = new MainFrame(controller);
    frame->Show(true);
    return true;
}
