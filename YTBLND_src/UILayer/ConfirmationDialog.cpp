/**
 * \file ConfirmationDialog.cpp
 * \brief Implementation for ConfirmationDialog.
 * \author Jasmine Kumar
 */

#include "ConfirmationDialog.hpp"
#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/font.h>

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------
ConfirmationDialog::ConfirmationDialog(wxWindow*       parent,
                                       const wxString& title,
                                       const wxString& message,
                                       const wxString& okLabel,
                                       const wxString& cancelLabel)
    : wxDialog(parent, wxID_ANY, title,
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
{
    // -- Overall panel background ---------------------------------------------
    SetBackgroundColour(UIColors::Background());

    // -- Message label --------------------------------------------------------
    wxStaticText* msgLabel = new wxStaticText(this, wxID_ANY, message,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxALIGN_CENTER_HORIZONTAL);
    msgLabel->SetForegroundColour(UIColors::TextPrimary());
    msgLabel->SetBackgroundColour(UIColors::Background());

    wxFont msgFont = msgLabel->GetFont();
    msgFont.SetPointSize(11);
    msgLabel->SetFont(msgFont);

    // -- OK button (Accent colour) ---------------------------------------------
    wxButton* okBtn = new wxButton(this, wxID_OK, okLabel,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxBORDER_NONE);
    UIButtons::ApplySizeBounds(okBtn, ButtonType::Small);
    okBtn->SetBackgroundColour(UIColors::Accent());
    okBtn->SetForegroundColour(UIColors::TextPrimary());

    okBtn->Bind(wxEVT_ENTER_WINDOW, [okBtn](wxMouseEvent& e) {
        okBtn->SetBackgroundColour(UIColors::AccentHover());
        okBtn->Refresh();
        e.Skip();
    });
    okBtn->Bind(wxEVT_LEAVE_WINDOW, [okBtn](wxMouseEvent& e) {
        okBtn->SetBackgroundColour(UIColors::Accent());
        okBtn->Refresh();
        e.Skip();
    });

    // -- Cancel button (SurfaceRaised) ----------------------------------------
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, cancelLabel,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxBORDER_NONE);
    UIButtons::ApplySizeBounds(cancelBtn, ButtonType::Small);
    cancelBtn->SetBackgroundColour(UIColors::SurfaceRaised());
    cancelBtn->SetForegroundColour(UIColors::TextPrimary());

    cancelBtn->Bind(wxEVT_ENTER_WINDOW, [cancelBtn](wxMouseEvent& e) {
        cancelBtn->SetBackgroundColour(wxColour(55, 55, 55));
        cancelBtn->Refresh();
        e.Skip();
    });
    cancelBtn->Bind(wxEVT_LEAVE_WINDOW, [cancelBtn](wxMouseEvent& e) {
        cancelBtn->SetBackgroundColour(UIColors::SurfaceRaised());
        cancelBtn->Refresh();
        e.Skip();
    });

    // -- Button row -----------------------------------------------------------
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(cancelBtn, 0, wxRIGHT, 12);
    btnSizer->Add(okBtn,     0);

    // -- Root sizer with generous padding for "rounded" feel ------------------
    wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);
    rootSizer->AddSpacer(24);
    rootSizer->Add(msgLabel,  0, wxEXPAND | wxLEFT | wxRIGHT, 32);
    rootSizer->AddSpacer(28);
    rootSizer->Add(btnSizer,  0, wxALIGN_RIGHT | wxRIGHT, 32);
    rootSizer->AddSpacer(24);

    SetSizer(rootSizer);
    Fit();
    SetMinSize(wxSize(340, -1));
    Fit(); // refit after applying min size

    if (parent) {
        CentreOnParent();
    } else {
        CentreOnScreen();
    }
}

// -----------------------------------------------------------------------------
// Static convenience helper
// -----------------------------------------------------------------------------
bool ConfirmationDialog::Confirm(wxWindow*       parent,
                                 const wxString& title,
                                 const wxString& message)
{
    ConfirmationDialog dlg(parent, title, message);
    return dlg.ShowModal() == wxID_OK;
}
