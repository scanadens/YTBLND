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
    // Enable JPEG and PNG handlers for thumbnail loading
    wxInitAllImageHandlers();

    auto* frame = new MainFrame(controller);
    frame->Show(true);
    return true;
}
