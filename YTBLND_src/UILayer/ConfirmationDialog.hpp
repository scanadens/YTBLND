/**
 * \file ConfirmationDialog.hpp
 * \brief Dark-themed modal confirmation dialog.
 * \author Jasmine Kumar
 */

// A simple two-button (OK / Cancel) dialog that matches the YTBLND dark theme.
// Use the static Confirm() helper for the common one-shot pattern, or
// construct manually when you need custom button labels.
//
// Typical usage (static helper):
//   if (ConfirmationDialog::Confirm(this, "Log Out", "Are you sure?")) { ... }
//
// Custom labels:
//   ConfirmationDialog dlg(this, "Delete Blend", "This cannot be undone.",
//                          "Delete", "Keep");
//   if (dlg.ShowModal() == wxID_OK) { ... }
//
// Styling notes:
//   - OK button uses UIColors::Accent (purple)
//   - Cancel button uses UIColors::SurfaceRaised (grey)
//   - Both buttons lighten slightly on hover
//
// TODO: Add a wxIcon / warning symbol to the left of the message for
//       destructive-action dialogs (delete, logout, etc.).

#pragma once
#include <wx/dialog.h>
#include <wx/string.h>

// A dark-themed modal confirmation dialog.
//
// Typical usage via the static helper:
//   if (ConfirmationDialog::Confirm(parent, "Delete blend?", "This cannot be undone.")) { ... }
//
// Or construct manually for custom button labels:
//   ConfirmationDialog dlg(parent, "Title", "Message", "Delete", "Keep");
//   if (dlg.ShowModal() == wxID_OK) { ... }
/**
 * \class ConfirmationDialog
 * \brief ConfirmationDialog class declaration.
 */
class ConfirmationDialog : public wxDialog {
public:
    ConfirmationDialog(wxWindow*       parent,
                       const wxString& title,
                       const wxString& message,
                       const wxString& okLabel     = wxT("OK"),
                       const wxString& cancelLabel = wxT("Cancel"));

    // Convenience: create, show, destroy - returns true if OK was clicked.
    static bool Confirm(wxWindow*       parent,
                        const wxString& title,
                        const wxString& message);
};
