#pragma once
#include <wx/dialog.h>
#include <wx/string.h>

// A dark-themed modal confirmation dialog.
//
// Typical usage via the static helper:
//   if (ConfirmationDialog::Confirm(parent, "Delete blend?", "This cannot be undone.")) { … }
//
// Or construct manually for custom button labels:
//   ConfirmationDialog dlg(parent, "Title", "Message", "Delete", "Keep");
//   if (dlg.ShowModal() == wxID_OK) { … }
class ConfirmationDialog : public wxDialog {
public:
    ConfirmationDialog(wxWindow*       parent,
                       const wxString& title,
                       const wxString& message,
                       const wxString& okLabel     = wxT("OK"),
                       const wxString& cancelLabel = wxT("Cancel"));

    // Convenience: create, show, destroy — returns true if OK was clicked.
    static bool Confirm(wxWindow*       parent,
                        const wxString& title,
                        const wxString& message);
};
