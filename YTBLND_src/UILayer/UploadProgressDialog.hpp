
/**
 * \file UploadProgressDialog.hpp
 * \author Shamar Pennant
 * \brief Dark-themed non-modal progress dialog for file upload/processing
 * Displays progress bar, percentage, and status message during long operations. 
 * Helping to indicate to the user the progress of file uploading
*/

#pragma once
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/string.h>
#include <wx/stattext.h>

/// \class UploadProgressDialog
/// \brief Non-modal progress dialog showing upload/processing progress
class UploadProgressDialog : public wxDialog {
public:
    /// \brief Constructor with parent window and title
    UploadProgressDialog(wxWindow*       parent,
                         const wxString& title);

    /// \brief Destructor
    ~UploadProgressDialog();

    /// \brief Update progress bar and status message
    /// \param progress Progress value 0.0 to 1.0
    /// \param message Optional status message to display
    void UpdateProgress(double progress, const wxString& message = wxEmptyString);

    /// \brief Show dialog non-modally
    virtual int ShowModal();

private:
    /// Track and fill are custom panels so Accent color is reliable across platforms.
    wxPanel*        progressTrack;  ///< Progress track background
    wxPanel*        progressFill;   ///< Progress fill foreground
    wxStaticText*   percentLabel;   ///< Percentage display
    wxStaticText*   messageLabel;   ///< Status message
    double          currentProgress = 0.0;

    void UpdateProgressFill();
};


