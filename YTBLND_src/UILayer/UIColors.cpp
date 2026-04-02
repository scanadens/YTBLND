// UIColors.cpp
#include "UIColors.hpp"

namespace {
void ApplyPalette(const Palette& p) {
    UIColors::Background = p.Background;
    UIColors::Surface = p.Surface;
    UIColors::SurfaceRaised = p.SurfaceRaised;
    UIColors::Accent = p.Accent;
    UIColors::AccentHover = p.AccentHover;
    UIColors::TextPrimary = p.TextPrimary;
    UIColors::TextSecondary = p.TextSecondary;
    UIColors::TextMuted = p.TextMuted;
    UIColors::Separator = p.Separator;
    UIColors::Danger = p.Danger;
}
}

Palette* UIColors::Current = &UIColors::DarkMode;

Palette UIColors::DarkMode = { 
    wxColour(13,13,13),     ///< #0D0D0D — window background.
    wxColour(25,25,25),     ///< #191919 — panel / card background.
    wxColour(38,38,38),     ///< #262626 — inputs, raised elements.

    wxColour(138,0,255),    ///< #8A00FF — brand purple.
    wxColour(160,40,255),   ///< #A028FF — lighter hover purple.

    wxColour(255,255,255),  ///< #FFFFFF - Body text.
    wxColour(160,160,160),  ///< #BDBDBD - Secondary text.
    wxColour(100,100,100),  ///< #757575 - Placeholder text.

    wxColour(50,50,50),     ///< #252525 - Rule-line colour.
    wxColour(211, 47, 47)   ///< #D32F2F — red destructive-action color.
};


Palette UIColors::LightMode = { 
    wxColour(245, 245, 245), ///< #F5F5F5 — window background.
    wxColour(255, 255, 255), ///< #FFFFFF — card background.
    wxColour(230, 230, 230), ///< #E6E6E6 — inputs, raised elements.

    wxColour(0, 102, 204),   ///< #0066CC — blue brand accent.
    wxColour(0, 120, 215),   ///< #0078D7 — blue hover feedback.

    wxColour(33, 33, 33),    ///< #212121 — body text.
    wxColour(117, 117, 117), ///< #757575 — secondary text.
    wxColour(189, 189, 189), ///< #BDBDBD — placeholder text.

    wxColour(224, 224, 224), ///< #E0E0E0 — rule-line color.
    wxColour(211, 47, 47)    ///< #D32F2F — red destructive-action color.
};


Palette UIColors::NeonMode = { 
    wxColour(0, 0, 0),       ///< #000000 — pure black background.
    wxColour(15, 15, 15),    ///< #0F0F0F — card background.
    wxColour(30, 30, 30),    ///< #1E1E1E — inputs, raised elements.

    wxColour(57, 255, 20),   ///< #39FF14 — neon accent.
    wxColour(102, 255, 0),   ///< #66FF00 — Hyper green for hover feedback.

    wxColour(255, 255, 255), ///< #FFFFFF — body text.
    wxColour(0, 255, 255),   ///< #00FFFF — secondary text.
    wxColour(0, 138, 138),   ///< #008A8A — placeholder text.

    wxColour(57, 255, 20),   ///< #39FF14 — rule-line color.
    wxColour(255, 0, 153)    ///< #FF0099 — neon pink destructive-action color.
};

wxColour UIColors::Background = UIColors::DarkMode.Background;
wxColour UIColors::Surface = UIColors::DarkMode.Surface;
wxColour UIColors::SurfaceRaised = UIColors::DarkMode.SurfaceRaised;
wxColour UIColors::Accent = UIColors::DarkMode.Accent;
wxColour UIColors::AccentHover = UIColors::DarkMode.AccentHover;
wxColour UIColors::TextPrimary = UIColors::DarkMode.TextPrimary;
wxColour UIColors::TextSecondary = UIColors::DarkMode.TextSecondary;
wxColour UIColors::TextMuted = UIColors::DarkMode.TextMuted;
wxColour UIColors::Separator = UIColors::DarkMode.Separator;
wxColour UIColors::Danger = UIColors::DarkMode.Danger;

static ThemeType sCurrentTheme = ThemeType::Dark;

ThemeType UIColors::GetCurrentTheme() {
    return sCurrentTheme;
}

void UIColors::SetTheme(ThemeType theme) {
    sCurrentTheme = theme;
    switch (theme) {
        case ThemeType::Light:
            Current = &LightMode;
            break;
        case ThemeType::Neon:
            Current = &NeonMode;
            break;
        case ThemeType::Dark:
        default:
            Current = &DarkMode;
            break;
    }

    ApplyPalette(*Current);
}
