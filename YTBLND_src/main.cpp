/**
 * \file main.cpp
 * \brief Implementation for main.
 * \author Jasmine Kumar
 */

// ============================================================================
// main.cpp - Application entry point
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
#include <wx/socket.h>

#include "UILayer/MainFrame.hpp"
#include "AppLayer/AppController.hpp"

class YTBLNDApp : public wxApp {
public:
    bool OnInit() override;
    int  OnExit() override;

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

    // Initialise wxSocket on the main thread before any background thread
    // calls wxURL::GetInputStream().  wxWidgets requires socket init to
    // happen on the main thread; failing to do this causes the
    // "wxWidgets sockets must be initialized in the main thread" error.
    wxSocketBase::Initialize();

    // MainFrame starts maximised and shows the LOGIN page.
    auto* frame = new MainFrame(controller);
    frame->Show(true);
    return true;
}

int YTBLNDApp::OnExit() {
    wxSocketBase::Shutdown();
    return wxApp::OnExit();
}
