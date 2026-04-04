/**
 * \file OperationProgressDialog.cpp
 * \brief Implementation of non-modal progress dialog.
 * \author Shamar Pennant
 */

#include "OperationProgressDialog.hpp"
#include "UIColors.hpp"

#include <wx/dcbuffer.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <iostream>

namespace {
constexpr int kProgressBorderPx = 1;
}

OperationProgressDialog::OperationProgressDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title,
               wxDefaultPosition, wxSize(420, 180),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {

    // Create main vertical sizer
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(10);

    // Status message label
    messageLabel = new wxStaticText(this, wxID_ANY, wxT("Initializing..."));
    wxFont messageFont = messageLabel->GetFont();
    messageFont.SetPointSize(10);
    messageLabel->SetFont(messageFont);
    messageLabel->SetForegroundColour(UIColors::TextPrimary());
    mainSizer->Add(messageLabel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // Custom progress bar (track + fill) to ensure theme colors render on GTK.
    progressTrack = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 14));
    progressTrack->SetMinSize(wxSize(-1, 14));
    progressTrack->SetBackgroundStyle(wxBG_STYLE_PAINT);
    progressTrack->SetBackgroundColour(UIColors::SurfaceRaised());

    // Paint a consistent border for visibility in all themes.
    progressTrack->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(progressTrack);
        const wxSize sz = progressTrack->GetClientSize();

        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(UIColors::Separator()));
        dc.DrawRectangle(0, 0, sz.GetWidth(), sz.GetHeight());

        const int innerW = std::max(0, sz.GetWidth() - (2 * kProgressBorderPx));
        const int innerH = std::max(0, sz.GetHeight() - (2 * kProgressBorderPx));
        dc.SetBrush(wxBrush(UIColors::SurfaceRaised()));
        dc.DrawRectangle(kProgressBorderPx, kProgressBorderPx, innerW, innerH);
    });

    progressFill = new wxPanel(progressTrack, wxID_ANY, wxDefaultPosition, wxSize(0, 14));
    progressFill->SetBackgroundColour(UIColors::Accent());

    progressTrack->Bind(wxEVT_SIZE, [this](wxSizeEvent& evt) {
        UpdateProgressFill();
        evt.Skip();
    });

    mainSizer->Add(progressTrack, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    // Percentage label centred below the progress bar.
    percentLabel = new wxStaticText(this, wxID_ANY, wxT("0%"),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTRE_HORIZONTAL);
    wxFont percentFont = percentLabel->GetFont();
    percentFont.SetPointSize(9);
    percentLabel->SetFont(percentFont);
    percentLabel->SetForegroundColour(UIColors::TextSecondary());
    mainSizer->AddSpacer(6);
    mainSizer->Add(percentLabel, 0, wxALIGN_CENTER_HORIZONTAL);
    mainSizer->AddStretchSpacer(1);

    SetSizer(mainSizer);
    SetBackgroundColour(UIColors::Surface());
    SetWindowStyle(wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    UpdateProgressFill();

    std::cout << "[OperationProgressDialog] Created progress dialog: "
              << std::string(title.mb_str()) << "\n";
}

OperationProgressDialog::~OperationProgressDialog() {
    std::cout << "[OperationProgressDialog] Destroyed progress dialog\n";
}

void OperationProgressDialog::UpdateProgress(double progress, const wxString& message) {
    // Clamp progress to 0.0-1.0 range
    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;

    // Update custom progress fill
    currentProgress = progress;
    UpdateProgressFill();

    // Update percentage label
    int percent = static_cast<int>(progress * 100);
    percentLabel->SetLabel(wxString::Format(wxT("%d%%"), percent));

    // Update message if provided
    if (!message.IsEmpty()) {
        messageLabel->SetLabel(message);
    }

    // Redraw dialog
    Layout();
    Refresh();
    wxYield();
}

int OperationProgressDialog::ShowModal() {
    // Display non-modally to keep application responsive
    Show(true);
    return wxID_OK;
}

void OperationProgressDialog::UpdateProgressFill() {
    if (progressTrack == nullptr || progressFill == nullptr) {
        return;
    }

    const wxSize trackSize = progressTrack->GetClientSize();
    const int innerW = std::max(0, trackSize.GetWidth() - (2 * kProgressBorderPx));
    const int innerH = std::max(0, trackSize.GetHeight() - (2 * kProgressBorderPx));
    const int fillWidth = static_cast<int>(innerW * currentProgress);

    progressFill->SetSize(kProgressBorderPx,
                          kProgressBorderPx,
                          std::max(0, fillWidth),
                          innerH);

    progressTrack->Refresh();
    progressFill->Refresh();
}
