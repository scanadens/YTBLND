#pragma once
#include <wx/wx.h>
#include "UIPages.h"

class AppController;

// Shown after login when the user has no YouTube data loaded yet.
// Explains how to obtain and place the Watch Later CSV, then lets the
// user browse for the file or skip and go straight to blend creation.
class DataInstructionsPanel : public wxPanel {
public:
    DataInstructionsPanel(wxWindow* parent, AppController& controller, NavigateFn nav);

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    void OnBrowse(wxCommandEvent&);
    void OnSkip  (wxCommandEvent&);
};
