// ============================================================================
// LoginPanel.cpp — Sign In / Register screen implementation
// ============================================================================

#include "LoginPanel.hpp"

#include <wx/dcbuffer.h>
#include <wx/simplebook.h>
#include <wx/statline.h>

#include "UIColors.hpp"
#include "ButtonsConfig.hpp"
#include "OperationProgressDialog.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"
#include "../AppLayer/EventRouter.hpp"
#include "../ModelLayer/User.hpp"

// -- Helpers -------------------------------------------------------------------

static wxTextCtrl* MakeField(wxWindow* parent,
                              const wxString& hint,
                              bool password = false)
{
    long style = wxTE_PROCESS_ENTER | (password ? wxTE_PASSWORD : 0);
    auto* ctrl = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition,
                                wxSize(-1, 36), style);
    ctrl->SetHint(hint);
    ctrl->SetBackgroundColour(UIColors::SurfaceRaised());
    ctrl->SetForegroundColour(UIColors::TextPrimary());
    return ctrl;
}

static wxStaticText* MakeError(wxWindow* parent)
{
    auto* lbl = new wxStaticText(parent, wxID_ANY, "",
                                 wxDefaultPosition, wxDefaultSize,
                                 wxALIGN_CENTER_HORIZONTAL);
    lbl->SetForegroundColour(UIColors::Danger());
    lbl->Hide();
    return lbl;
}

// -- Construction --------------------------------------------------------------

LoginPanel::LoginPanel(wxWindow* parent, AppController& controller, NavigateFn nav,
                       const wxImage& bgImage)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_nav(std::move(nav))
    , m_bgImage(bgImage)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &LoginPanel::OnPaint, this);

    // -- Outer centering sizer -------------------------------------------------
    auto* outer = new wxBoxSizer(wxVERTICAL);

    // -- YTBLND title ----------------------------------------------------------
    auto* titleLabel = new wxStaticText(this, wxID_ANY, "YTBLND",
                                        wxDefaultPosition, wxDefaultSize,
                                        wxALIGN_CENTRE_HORIZONTAL);
    wxFont tf = titleLabel->GetFont();
    tf.SetPointSize(42);
    tf.SetWeight(wxFONTWEIGHT_BOLD);
    titleLabel->SetFont(tf);
    titleLabel->SetForegroundColour(UIColors::Accent());

    auto* subtitle = new wxStaticText(this, wxID_ANY, "Your blend, your music.",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxALIGN_CENTRE_HORIZONTAL);
    subtitle->SetForegroundColour(UIColors::TextSecondary());

    outer->AddStretchSpacer(2);
    outer->Add(titleLabel, 0, wxALIGN_CENTER | wxBOTTOM, 4);
    outer->Add(subtitle,   0, wxALIGN_CENTER | wxBOTTOM, 32);

    // -- Form card -------------------------------------------------------------
    auto* card = new wxPanel(this, wxID_ANY);
    card->SetBackgroundColour(UIColors::Surface());
    card->SetMinSize(wxSize(400, -1));
    auto* cardSizer = new wxBoxSizer(wxVERTICAL);

    // Tab strip
    auto* tabRow = new wxBoxSizer(wxHORIZONTAL);
    m_signinTab   = new wxButton(card, wxID_ANY, "Sign In",
                                  wxDefaultPosition, wxDefaultSize);
    m_registerTab = new wxButton(card, wxID_ANY, "Register",
                                  wxDefaultPosition, wxDefaultSize);
    for (auto* btn : {m_signinTab, m_registerTab}) {
        UIButtons::ApplySizeBounds(btn, ButtonType::FormTab);
        btn->SetBackgroundColour(UIColors::SurfaceRaised());
        btn->SetForegroundColour(UIColors::TextSecondary());
    }
    tabRow->Add(m_signinTab,   1, wxEXPAND);
    tabRow->Add(m_registerTab, 1, wxEXPAND);
    cardSizer->Add(tabRow, 0, wxEXPAND);

    // Divider below tabs
    auto* div = new wxStaticLine(card, wxID_ANY);
    div->SetBackgroundColour(UIColors::Separator());
    cardSizer->Add(div, 0, wxEXPAND);

    // Form pages
    m_formBook = new wxSimplebook(card, wxID_ANY);
    m_formBook->SetBackgroundColour(UIColors::Surface());

    // Page 0: Sign In
    {
        auto* page  = new wxPanel(m_formBook, wxID_ANY);
        page->SetBackgroundColour(UIColors::Surface());
        auto* sz = new wxBoxSizer(wxVERTICAL);
        BuildSignInForm(page, sz);
        page->SetSizer(sz);
        m_formBook->AddPage(page, "SignIn");
    }

    // Page 1: Register
    {
        auto* page  = new wxPanel(m_formBook, wxID_ANY);
        page->SetBackgroundColour(UIColors::Surface());
        auto* sz = new wxBoxSizer(wxVERTICAL);
        BuildRegisterForm(page, sz);
        page->SetSizer(sz);
        m_formBook->AddPage(page, "Register");
    }

    cardSizer->Add(m_formBook, 0, wxEXPAND);
    card->SetSizer(cardSizer);

    // Centre the card horizontally
    auto* hCentre = new wxBoxSizer(wxHORIZONTAL);
    hCentre->AddStretchSpacer(1);
    hCentre->Add(card, 0, wxEXPAND);
    hCentre->AddStretchSpacer(1);

    outer->Add(hCentre, 0, wxEXPAND);
    outer->AddStretchSpacer(3);
    SetSizer(outer);

    // -- Tab bindings ----------------------------------------------------------
    m_signinTab  ->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ ShowTab(0); });
    m_registerTab->Bind(wxEVT_BUTTON, [this](wxCommandEvent&){ ShowTab(1); });

    ShowTab(0);  // start on Sign In

    // Load the themed background and re-load on theme switch
    LoadThemedBackground();
    m_controller.getEventRouter().registerListener("theme_bg_login",
        [this](const EventPayload&) { LoadThemedBackground(); Refresh(); });
}

// -- Form builders -------------------------------------------------------------

void LoginPanel::BuildSignInForm(wxWindow* parent, wxSizer* sizer)
{
    m_siUsername = MakeField(parent, "Username");
    m_siPassword = MakeField(parent, "Password", true);
    m_siError    = MakeError(parent);

    auto* btn = new wxButton(parent, wxID_ANY, "Sign In",
                              wxDefaultPosition, wxDefaultSize);
    UIButtons::ApplySizeBounds(btn, ButtonType::FormSubmit);
    btn->SetBackgroundColour(UIColors::Accent());
    btn->SetForegroundColour(UIColors::TextPrimary());

    sizer->Add(m_siUsername, 0, wxEXPAND | wxCENTRE | wxALL, 8);
    sizer->Add(m_siPassword, 0, wxEXPAND | wxCENTRE | wxALL, 8);
    sizer->Add(m_siError,    0, wxEXPAND | wxTOP | wxBOTTOM, 8);
    sizer->Add(btn,          0, wxEXPAND | wxALL | wxCENTER, 20);

    btn->Bind(wxEVT_BUTTON, &LoginPanel::OnSignIn, this);
    // Also allow Enter in password field to submit
    m_siPassword->Bind(wxEVT_TEXT_ENTER,
        [this](wxCommandEvent& e){ OnSignIn(e); });
}

void LoginPanel::BuildRegisterForm(wxWindow* parent, wxSizer* sizer)
{
    m_regUsername = MakeField(parent, "Username  (this will be your login ID)");
    m_regEmail    = MakeField(parent, "Email (optional)");
    m_regPassword = MakeField(parent, "Password", true);
    m_regConfirm  = MakeField(parent, "Confirm password", true);
    m_regError    = MakeError(parent);

    auto* btn = new wxButton(parent, wxID_ANY, "Create Account",
                              wxDefaultPosition, wxDefaultSize);
    UIButtons::ApplySizeBounds(btn, ButtonType::FormSubmit);
    btn->SetBackgroundColour(UIColors::Accent());
    btn->SetForegroundColour(UIColors::TextPrimary());

    sizer->Add(m_regUsername, 0, wxEXPAND | wxALL, 8);
    sizer->Add(m_regEmail,    0, wxEXPAND | wxALL, 8);
    sizer->Add(m_regPassword, 0, wxEXPAND | wxALL, 8);
    sizer->Add(m_regConfirm,  0, wxEXPAND | wxALL, 8);
    sizer->Add(m_regError,   0, wxEXPAND | wxTOP | wxBOTTOM, 8);
    sizer->Add(btn,           0, wxEXPAND | wxALL | wxCENTER, 20);

    btn->Bind(wxEVT_BUTTON, &LoginPanel::OnRegister, this);
}

// -- Tab switcher --------------------------------------------------------------

void LoginPanel::ShowTab(int index)
{
    m_formBook->SetSelection(index);

    // Active tab: accent colour; inactive: muted
    auto styleActive = [](wxButton* btn) {
        btn->SetBackgroundColour(UIColors::Accent());
        btn->SetForegroundColour(UIColors::TextPrimary());
    };
    auto styleInactive = [](wxButton* btn) {
        btn->SetBackgroundColour(UIColors::SurfaceRaised());
        btn->SetForegroundColour(UIColors::TextSecondary());
    };

    if (index == 0) { styleActive(m_signinTab);   styleInactive(m_registerTab); }
    else            { styleInactive(m_signinTab);  styleActive(m_registerTab);   }

    m_siError ->Hide();
    m_regError->Hide();
    Layout();
}

// -- Event handlers ------------------------------------------------------------

void LoginPanel::OnSignIn(wxCommandEvent& /*evt*/)
{
    std::string username = m_siUsername->GetValue().Trim().ToStdString();
    std::string password = m_siPassword->GetValue().ToStdString();

    if (username.empty() || password.empty()) {
        m_siError->SetLabel("Please enter your username and password.");
        m_siError->Show();
        m_siError->GetParent()->Layout();
        Layout();
        return;
    }

    OperationProgressDialog progress(this, "Signing In");
    progress.ShowModal();
    progress.UpdateProgress(0.02, "Preparing login...");

    m_controller.setProgressReporter([&progress](double ratio, const std::string& message) {
        progress.UpdateProgress(ratio, wxString::FromUTF8(message));
    });

    // Dispatch uses userID — username IS the userID in this app
    m_controller.getEventRouter().dispatch("login",
        {{"userID", username}, {"password", password}});
    m_controller.clearProgressReporter();

    progress.UpdateProgress(1.0, "Login finished");
    progress.Destroy();

    if (AppState::getInstance().getCurrentUser() == nullptr) {
        m_siError->SetLabel("Incorrect username or password.");
        m_siError->Show();
        m_siError->GetParent()->Layout();
        m_siPassword->Clear();
        Layout();
        return;
    }

    m_siError->Hide();
    ProceedAfterLogin();
}

void LoginPanel::OnRegister(wxCommandEvent& /*evt*/)
{
    std::string username = m_regUsername->GetValue().Trim().ToStdString();
    std::string email    = m_regEmail   ->GetValue().Trim().ToStdString();
    std::string password = m_regPassword->GetValue().ToStdString();
    std::string confirm  = m_regConfirm ->GetValue().ToStdString();

    if (username.empty() || password.empty()) {
        m_regError->SetLabel("Username and password are required.");
        m_regError->Show(); Layout(); return;
    }
    if (password != confirm) {
        m_regError->SetLabel("Passwords do not match.");
        m_regError->Show(); Layout(); return;
    }
    if (password.size() < 6) {
        m_regError->SetLabel("Password must be at least 6 characters.");
        m_regError->Show(); Layout(); return;
    }

    // userID == username in this app
    m_controller.getEventRouter().dispatch("register",
        {{"userID",   username},
         {"username", username},
         {"email",    email},
         {"password", password}});

    if (AppState::getInstance().getCurrentUser() == nullptr) {
        m_regError->SetLabel("Username already taken. Choose another.");
        m_regError->Show(); Layout(); return;
    }

    m_regError->Hide();
    ProceedAfterLogin();
}

// -- Post-login routing --------------------------------------------------------

void LoginPanel::ProceedAfterLogin()
{
    User* user = AppState::getInstance().getCurrentUser();
    if (!user) return;

    // If a blend was restored by handleLogin, go straight to the feed
    if (AppState::getInstance().getActiveBlend() != nullptr) {
        m_nav(Page::HOME);
        return;
    }

    // If the user has no Watch Later data, send them to the upload screen.
    // Show an info dialog first if they were added to a blend by someone else.
    bool hasData = !user->getYouTubeData().getWatchLaterVideos().empty();
    if (!hasData) {
        std::string msg = AppState::getInstance().takePendingBlendMessage();
        if (!msg.empty())
            wxMessageBox(msg, "You're in a Blend!", wxOK | wxICON_INFORMATION, nullptr);
        m_nav(Page::DATA_INSTRUCTIONS);
        return;
    }

    // Has data but no blend yet — let them create one
    m_nav(Page::BLEND_CREATION);
}

// -- Reset ---------------------------------------------------------------------

void LoginPanel::Refresh()
{
    Reset();
}

void LoginPanel::Reset()
{
    m_siUsername->Clear();
    m_siPassword->Clear();
    m_regUsername->Clear();
    m_regEmail->Clear();
    m_regPassword->Clear();
    m_regConfirm->Clear();
    m_siError ->Hide();
    m_regError->Hide();
    ShowTab(0);
}

// -- Themed background --------------------------------------------------------

void LoginPanel::LoadThemedBackground() {
    // Map theme name to background image filename
    wxString themeName = UIColors::Current ? UIColors::Current->Name : wxString("Dark Mode");
    wxString imageFile;

    if (themeName == "Light Mode")
        imageFile = "checkered_wave_background.jpg";
    else if (themeName == "Neon Mode")
        imageFile = "purple_neon_synth.jpg";
    else
        imageFile = "dark_purple_stripes.jpg";

    // Try to find the file
    for (const auto& candidate : wxArrayString{
             "YTBLND_src/resources/" + imageFile,
             "resources/" + imageFile,
             "../resources/" + imageFile}) {
        if (wxFileExists(candidate)) {
            wxImage img(candidate, wxBITMAP_TYPE_JPEG);
            if (img.IsOk()) {
                m_bgImage = img;
                return;
            }
        }
    }
    // If not found, clear so OnPaint falls back to solid color
    m_bgImage = wxNullImage;
}

// -- Background painting -------------------------------------------------------

void LoginPanel::OnPaint(wxPaintEvent&)
{
    wxBufferedPaintDC dc(this);
    if (m_bgImage.IsOk()) {
        wxSize sz = GetClientSize();
        if (sz.x > 0 && sz.y > 0) {
            wxBitmap scaled(m_bgImage.Scale(sz.x, sz.y, wxIMAGE_QUALITY_HIGH));
            dc.DrawBitmap(scaled, 0, 0, false);
            return;
        }
    }
    dc.SetBackground(wxBrush(UIColors::Background()));
    dc.Clear();
}
