#include "ActiveBlendsPanel.hpp"

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "TopBar.hpp"
#include "ConfirmationDialog.hpp"
#include "UploadProgressDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"

ActiveBlendsPanel::ActiveBlendsPanel(wxWindow* parent,
                                     AppController& controller,
                                     NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background());

    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* topBar = new TopBar(this, "My Blends", m_nav, Page::HOME);
    root->Add(topBar, 0, wxEXPAND);

    // Scrollable blend list
    m_listScroll = new wxScrolledWindow(this, wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         wxVSCROLL | wxBORDER_NONE);
    m_listScroll->SetBackgroundColour(UIColors::Background());
    m_listScroll->SetScrollRate(0, 12);

    m_listInner = new wxPanel(m_listScroll, wxID_ANY);
    m_listInner->SetBackgroundColour(UIColors::Background());
    m_listSizer = new wxBoxSizer(wxVERTICAL);
    m_listInner->SetSizer(m_listSizer);

    auto* scrollSizer = new wxBoxSizer(wxVERTICAL);
    scrollSizer->Add(m_listInner, 1, wxEXPAND);
    m_listScroll->SetSizer(scrollSizer);

    root->Add(m_listScroll, 1, wxEXPAND | wxALL, 8);

    // Empty state label
    m_emptyLabel = new wxStaticText(m_listInner, wxID_ANY,
                                    "No active blends.",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    m_emptyLabel->SetForegroundColour(UIColors::TextMuted());
    m_listSizer->Add(m_emptyLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 24);

    SetSizer(root);
}

void ActiveBlendsPanel::Refresh()
{
    RebuildList();
}

void ActiveBlendsPanel::RebuildList()
{
    m_listSizer->Clear(true);

    // Re-create empty label (cleared above)
    m_emptyLabel = new wxStaticText(m_listInner, wxID_ANY,
                                    "No active blends.",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    m_emptyLabel->SetForegroundColour(UIColors::TextMuted());

    auto blends = m_controller.fetchUserBlends();

    if (blends.empty()) {
        m_emptyLabel->Show(true);
        m_listSizer->Add(m_emptyLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 24);
    } else {
        m_emptyLabel->Show(false);
        m_listSizer->Add(m_emptyLabel, 0);

        for (const auto& blend : blends) {
            auto* row = new wxPanel(m_listInner, wxID_ANY);
            row->SetBackgroundColour(UIColors::SurfaceRaised());
            row->SetMinSize(wxSize(-1, 52));

            auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

            // Blend title label
            auto* titleLabel = new wxStaticText(row, wxID_ANY,
                                                 wxString::FromUTF8(blend.title));
            titleLabel->SetForegroundColour(UIColors::TextPrimary());
            wxFont f = titleLabel->GetFont();
            f.SetPointSize(12);
            f.SetWeight(wxFONTWEIGHT_BOLD);
            titleLabel->SetFont(f);

            // Open button
            auto* openBtn = new wxButton(row, wxID_ANY, "Open");
            UIButtons::ApplySizeBounds(openBtn, ButtonType::Compact);
            openBtn->SetBackgroundColour(UIColors::Accent());
            openBtn->SetForegroundColour(UIColors::TextPrimary());

            // Leave button
            auto* leaveBtn = new wxButton(row, wxID_ANY, "Leave");
            UIButtons::ApplySizeBounds(leaveBtn, ButtonType::Compact);
            leaveBtn->SetBackgroundColour(UIColors::Danger());
            leaveBtn->SetForegroundColour(UIColors::TextPrimary());

            std::string capID = blend.blendID;
            std::string capTitle = blend.title;
            openBtn->Bind(wxEVT_BUTTON, [this, capID, capTitle](wxCommandEvent&) {
                OnSelectBlend(capID, capTitle);
            });
            leaveBtn->Bind(wxEVT_BUTTON, [this, capID, capTitle](wxCommandEvent&) {
                OnLeaveBlend(capID, capTitle);
            });

            rowSizer->Add(titleLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
            rowSizer->Add(openBtn,    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
            rowSizer->Add(leaveBtn,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
            row->SetSizer(rowSizer);

            m_listSizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);
        }
    }

    m_listInner->Layout();
    m_listScroll->FitInside();
    m_listScroll->Layout();
    Layout();
}

void ActiveBlendsPanel::OnSelectBlend(const std::string& blendID,
                                       const std::string& title)
{
    UploadProgressDialog progress(this, "Loading Blend");
    progress.ShowModal();
    progress.UpdateProgress(0.02, "Preparing...");

    // Wire up the progress reporter so AppController updates the bar as it works
    m_controller.setProgressReporter([&progress](double ratio, const std::string& message) {
        progress.UpdateProgress(ratio, wxString::FromUTF8(message));
    });

    m_controller.getEventRouter().dispatch("selectBlend",
        {{"blendID", blendID}, {"blendTitle", title}});
    m_controller.clearProgressReporter();

    progress.UpdateProgress(1.0, "Blend ready");
    progress.Destroy();
    m_nav(Page::HOME);
}

void ActiveBlendsPanel::OnLeaveBlend(const std::string& blendID,
                                      const std::string& title)
{
    bool confirmed = ConfirmationDialog::Confirm(
        this, "Leave Blend",
        "Are you sure you want to leave \"" + title + "\"?");
    if (!confirmed) return;

    m_controller.getEventRouter().dispatch("leaveBlend",
        {{"blendID", blendID}, {"blendTitle", title}});

    RebuildList();
}
