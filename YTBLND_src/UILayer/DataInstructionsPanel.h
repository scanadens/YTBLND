// ============================================================================
// DataInstructionsPanel.h — Google Takeout CSV setup screen
//
// Shown after login when the user has no YouTube Watch Later data.
// Walks them through exporting from Google Takeout and loading the CSV.
//
// LAYOUT
// ──────
//   "Set Up Your YouTube Data" heading
//   Subtitle explaining why data is needed
//   Steps card (Surface, centred):
//     Numbered step-by-step instructions for Google Takeout export
//     Horizontal divider
//     "Browse for CSV..."  button (Accent)
//     "Skip for now"       button (SurfaceRaised / muted)
//
// CSV UPLOAD
// ──────────
//   OnBrowse: opens wxFileDialog (CSV filter), dispatches "uploadData" with
//   {filePath, userID} from AppState, then navigates to BLEND_CREATION.
//   TODO: Display a loading indicator while parsing — large CSV files may
//         take a noticeable amount of time.
//   TODO: Show an error/success message if the upload fails or the CSV is
//         malformed, rather than always navigating to BLEND_CREATION.
//
// SKIP
// ────
//   OnSkip: navigates directly to BLEND_CREATION without uploading.
//   The user can still create a blend (the algorithm will have less/no
//   personal data to work with).
//   TODO: Remind the user on the blend creation screen that they skipped
//         data upload, so the blend may not be personalised.
// ============================================================================

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
