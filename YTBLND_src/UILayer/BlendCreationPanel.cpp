// ============================================================================
// BlendCreationPanel.cpp — New-blend setup screen implementation
// ============================================================================

#include "BlendCreationPanel.hpp"

#include <wx/scrolwin.h>
#include <wx/textdlg.h>
#include <wx/statline.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "UIPages.hpp"
#include "UploadProgressDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"

// ── Construction ──────────────────────────────────────────────────────────────

BlendCreationPanel::BlendCreationPanel(wxWindow* parent,
                                       AppController& controller,
                                       NavigateFn nav)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
{
    SetBackgroundColour(UIColors::Background);

    auto* root = new wxBoxSizer(wxVERTICAL);

    // ── TopBar (dynamic back: LOGIN if no blend yet, HOME otherwise) ─────────
    auto* topBar = new wxPanel(this, wxID_ANY);
    topBar->SetBackgroundColour(UIColors::Surface);
    topBar->SetMinSize(wxSize(-1, 48));
    {
        auto* backBtn = new wxButton(topBar, wxID_ANY, wxT("< Back"));
        backBtn->SetBackgroundColour(UIColors::SurfaceRaised);
        backBtn->SetForegroundColour(UIColors::TextPrimary);
        UIButtons::ApplySizeBounds(backBtn, ButtonType::TopBarBack);

        backBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (AppState::getInstance().getActiveBlend() != nullptr)
                m_nav(Page::HOME);
            else
                m_nav(Page::LOGIN);
        });

        auto* titleLabel = new wxStaticText(topBar, wxID_ANY, "New Blend",
                                            wxDefaultPosition, wxDefaultSize,
                                            wxALIGN_CENTER_HORIZONTAL);
        titleLabel->SetForegroundColour(UIColors::TextPrimary);
        wxFont tf = titleLabel->GetFont();
        tf.SetPointSize(14);
        tf.SetWeight(wxFONTWEIGHT_BOLD);
        titleLabel->SetFont(tf);

        auto* barSizer = new wxBoxSizer(wxHORIZONTAL);
        barSizer->Add(backBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        barSizer->AddStretchSpacer(1);
        barSizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);
        barSizer->AddStretchSpacer(1);
        barSizer->Add(UIButtons::GetSize(ButtonType::TopBarBack).GetWidth() + 10, 0);

        auto* outerBar = new wxBoxSizer(wxVERTICAL);
        outerBar->Add(barSizer, 1, wxEXPAND | wxTOP | wxBOTTOM, 8);
        topBar->SetSizer(outerBar);
    }
    root->Add(topBar, 0, wxEXPAND);

    // ── Blend name row ──────────────────────────────────────────────────────────
    auto* namePanel = new wxPanel(this, wxID_ANY);
    namePanel->SetBackgroundColour(UIColors::Surface);
    auto* nameSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* nameLabel = new wxStaticText(namePanel, wxID_ANY, "Blend Name:");
    nameLabel->SetForegroundColour(UIColors::TextPrimary);
    wxFont nlf = nameLabel->GetFont();
    nlf.SetWeight(wxFONTWEIGHT_BOLD);
    nlf.SetPointSize(11);
    nameLabel->SetFont(nlf);

    m_blendNameCtrl = new wxTextCtrl(namePanel, wxID_ANY, "",
                                     wxDefaultPosition, wxSize(-1, 36),
                                     wxTE_PROCESS_ENTER);
    m_blendNameCtrl->SetHint("Enter a name for your blend...");
    m_blendNameCtrl->SetBackgroundColour(UIColors::SurfaceRaised);
    m_blendNameCtrl->SetForegroundColour(UIColors::TextPrimary);

    nameSizer->Add(nameLabel,       0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    nameSizer->Add(m_blendNameCtrl, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 12);
    namePanel->SetSizer(nameSizer);
    namePanel->SetMinSize(wxSize(-1, 48));
    root->Add(namePanel, 0, wxEXPAND);

    // ── Action row ────────────────────────────────────────────────────────────
    auto* actionPanel = new wxPanel(this, wxID_ANY);
    actionPanel->SetBackgroundColour(UIColors::Surface);
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* blendLabel = new wxStaticText(actionPanel, wxID_ANY, "PARTICIPANTS");
    blendLabel->SetForegroundColour(UIColors::TextPrimary);
    wxFont lf = blendLabel->GetFont();
    lf.SetWeight(wxFONTWEIGHT_BOLD);
    lf.SetPointSize(13);
    blendLabel->SetFont(lf);

    auto* addBtn = new wxButton(actionPanel, wxID_ANY, "Add");
    UIButtons::ApplySizeBounds(addBtn, ButtonType::Compact);
    addBtn->SetBackgroundColour(UIColors::SurfaceRaised);
    addBtn->SetForegroundColour(UIColors::TextPrimary);

    m_createBtn = new wxButton(actionPanel, wxID_ANY, "Create Blend");
    UIButtons::ApplySizeBounds(m_createBtn, ButtonType::Medium);
    m_createBtn->SetBackgroundColour(UIColors::Accent);
    m_createBtn->SetForegroundColour(UIColors::TextPrimary);
    m_createBtn->Disable();

    actionSizer->Add(blendLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 16);
    actionSizer->AddStretchSpacer(1);
    actionSizer->Add(addBtn,        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    actionSizer->Add(m_createBtn,   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
    actionPanel->SetSizer(actionSizer);
    actionPanel->SetMinSize(wxSize(-1, 52));

    root->Add(actionPanel, 0, wxEXPAND);

    // ── User list area ────────────────────────────────────────────────────────
    m_userListScroll = new wxScrolledWindow(this, wxID_ANY,
                                            wxDefaultPosition, wxDefaultSize,
                                            wxVSCROLL | wxBORDER_NONE);
    m_userListScroll->SetBackgroundColour(UIColors::Background);
    m_userListScroll->SetScrollRate(0, 12);

    m_userListInner = new wxPanel(m_userListScroll, wxID_ANY);
    m_userListInner->SetBackgroundColour(UIColors::Background);
    m_userListSizer = new wxBoxSizer(wxVERTICAL);

    // "No users added" placeholder
    m_emptyLabel = new wxStaticText(m_userListInner, wxID_ANY,
                                    "No users added",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    m_emptyLabel->SetForegroundColour(UIColors::TextMuted);
    m_userListSizer->Add(m_emptyLabel, 0,
                         wxALIGN_CENTER_HORIZONTAL | wxTOP, 24);

    m_userListInner->SetSizer(m_userListSizer);

    // Wire the inner panel into the scroll window
    auto* scrollSizer = new wxBoxSizer(wxVERTICAL);
    scrollSizer->Add(m_userListInner, 1, wxEXPAND);
    m_userListScroll->SetSizer(scrollSizer);

    root->Add(m_userListScroll, 1, wxEXPAND | wxALL, 8);

    // ── Count indicator ───────────────────────────────────────────────────────
    m_countLabel = new wxStaticText(this, wxID_ANY, "0 / 8 users",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    m_countLabel->SetForegroundColour(UIColors::TextSecondary);
    root->Add(m_countLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 12);

    SetSizer(root);

    // ── Bindings ──────────────────────────────────────────────────────────────
    addBtn->Bind(wxEVT_BUTTON,      &BlendCreationPanel::OnAdd,    this);
    m_createBtn->Bind(wxEVT_BUTTON, &BlendCreationPanel::OnCreate, this);
}

// ── Public ────────────────────────────────────────────────────────────────────

void BlendCreationPanel::Refresh()
{
    Reload();
}

void BlendCreationPanel::Reload()
{
    m_addedUsers.clear();
    m_blendNameCtrl->Clear();

    // Pre-populate with the logged-in user so they don't have to add themselves
    User* current = AppState::getInstance().getCurrentUser();
    if (current)
        m_addedUsers.push_back(current->getUserID());

    RebuildUserList();
    UpdateCountLabel();

    // Button enabled once there are >= 2 users (current user + at least one other)
    if (m_addedUsers.size() >= 2)
        m_createBtn->Enable();
    else
        m_createBtn->Disable();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void BlendCreationPanel::RebuildUserList()
{
    // Destroy all children of the inner panel except the empty label.
    // We recreate the inner sizer from scratch.
    m_userListSizer->Clear(true /* delete_windows */);

    // Re-create the empty label (it was deleted above)
    m_emptyLabel = new wxStaticText(m_userListInner, wxID_ANY,
                                    "No users added",
                                    wxDefaultPosition, wxDefaultSize,
                                    wxALIGN_CENTER_HORIZONTAL);
    m_emptyLabel->SetForegroundColour(UIColors::TextMuted);

    if (m_addedUsers.empty()) {
        m_emptyLabel->Show(true);
        m_userListSizer->Add(m_emptyLabel, 0,
                             wxALIGN_CENTER_HORIZONTAL | wxTOP, 24);
    } else {
        m_emptyLabel->Show(false);
        m_userListSizer->Add(m_emptyLabel, 0); // keep in sizer but hidden

        for (const auto& username : m_addedUsers) {
            // Row panel
            auto* row = new wxPanel(m_userListInner, wxID_ANY);
            row->SetBackgroundColour(UIColors::SurfaceRaised);
            row->SetMinSize(wxSize(-1, 44));

            auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

            auto* nameLabel = new wxStaticText(row, wxID_ANY,
                                               wxString::FromUTF8(username));
            nameLabel->SetForegroundColour(UIColors::TextPrimary);

            auto* removeBtn = new wxButton(row, wxID_ANY, wxT("\xc3\x97"));
            UIButtons::ApplySizeBounds(removeBtn, ButtonType::Icon);
            removeBtn->SetBackgroundColour(UIColors::SurfaceRaised);
            removeBtn->SetForegroundColour(UIColors::Danger);

            // Capture username by value for the lambda
            std::string cap = username;
            removeBtn->Bind(wxEVT_BUTTON, [this, cap](wxCommandEvent&) {
                OnRemoveUser(cap);
            });

            rowSizer->Add(nameLabel,  0, wxALIGN_CENTER_VERTICAL | wxLEFT,  16);
            rowSizer->AddStretchSpacer(1);
            rowSizer->Add(removeBtn,  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
            row->SetSizer(rowSizer);

            m_userListSizer->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP,
                                 6);
        }
    }

    m_userListInner->Layout();
    m_userListScroll->FitInside();
    m_userListScroll->Layout();
}

void BlendCreationPanel::UpdateCountLabel()
{
    wxString txt = wxString::Format("%d / 8 users",
                                    static_cast<int>(m_addedUsers.size()));
    m_countLabel->SetLabel(txt);
}

// ── Event handlers ────────────────────────────────────────────────────────────

void BlendCreationPanel::OnAdd(wxCommandEvent& /*evt*/)
{
    if (static_cast<int>(m_addedUsers.size()) >= 8) {
        wxMessageBox("Maximum 8 users reached.", "Limit Reached",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxTextEntryDialog dlg(this, "Enter username:", "Add User");
    dlg.SetBackgroundColour(UIColors::Surface);
    if (dlg.ShowModal() != wxID_OK) return;

    std::string entered = dlg.GetValue().Trim().ToStdString();

    if (entered.empty()) return;

    // Check for duplicate
    for (const auto& u : m_addedUsers) {
        if (u == entered) {
            wxMessageBox("User \"" + entered + "\" is already in the list.",
                         "Duplicate User", wxOK | wxICON_WARNING, this);
            return;
        }
    }

    if (!m_controller.lookupUser(entered)) {
        wxMessageBox("User \"" + entered + "\" does not exist.\n"
                     "Please ask them to create an account first.",
                     "User Not Found", wxOK | wxICON_ERROR, this);
        return;
    }

    m_addedUsers.push_back(entered);
    RebuildUserList();
    UpdateCountLabel();

    if (m_addedUsers.size() >= 2)
        m_createBtn->Enable();
    else
        m_createBtn->Disable();
}

void BlendCreationPanel::OnCreate(wxCommandEvent& /*evt*/)
{
    if (m_addedUsers.size() < 2) return;

    std::string blendName = m_blendNameCtrl->GetValue().ToStdString();
    if (blendName.empty()) {
        wxMessageBox("Please enter a name for your blend.",
                     "Name Required", wxOK | wxICON_INFORMATION, this);
        m_blendNameCtrl->SetFocus();
        return;
    }

    // Build payload: {"blendTitle": "...", "userID_0": "alice", "userID_1": "bob", ...}
    EventPayload payload;
    payload["blendTitle"] = blendName;
    for (std::size_t i = 0; i < m_addedUsers.size(); ++i)
        payload["userID_" + std::to_string(i)] = m_addedUsers[i];

    UploadProgressDialog progress(this, "Creating Blend");
    progress.ShowModal();
    progress.UpdateProgress(0.02, "Preparing...");

    // Wire up the progress reporter so AppController updates the bar as it works
    m_controller.setProgressReporter([&progress](double ratio, const std::string& message) {
        progress.UpdateProgress(ratio, wxString::FromUTF8(message));
    });

    m_controller.getEventRouter().dispatch("createBlend", payload);
    m_controller.clearProgressReporter();

    progress.UpdateProgress(1.0, "Blend ready");
    progress.Destroy();

    // Check if any participants were missing data
    std::vector<std::string> missing = AppState::getInstance().getUsersWithoutData();
    if (!missing.empty()) {
        std::string msg = "The following users have no Watch Later data uploaded:\n";
        for (const auto& uid : missing)
            msg += "  \u2022 " + uid + "\n";

        if (AppState::getInstance().getActiveBlend() != nullptr) {
            // Blend was still created from whoever had data — warn and continue
            msg += "\nTheir videos were not included. The blend was created with the remaining users.";
            wxMessageBox(msg, "Some Users Missing Data", wxOK | wxICON_WARNING, this);
            m_nav(Page::HOME);
        } else {
            // No one had data — cannot create a blend at all
            msg += "\nNo blend could be created. Please ensure at least one user has uploaded their data.";
            wxMessageBox(msg, "Cannot Create Blend", wxOK | wxICON_ERROR, this);
        }
        return;
    }

    m_nav(Page::HOME);
}

void BlendCreationPanel::OnRemoveUser(const std::string& username)
{
    auto it = std::find(m_addedUsers.begin(), m_addedUsers.end(), username);
    if (it != m_addedUsers.end())
        m_addedUsers.erase(it);

    RebuildUserList();
    UpdateCountLabel();

    if (m_addedUsers.size() >= 2)
        m_createBtn->Enable();
    else
        m_createBtn->Disable();
}
