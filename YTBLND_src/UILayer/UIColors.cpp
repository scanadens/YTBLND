/**
 * \file UIColors.cpp
 * \brief Logic for dynamic theme switching and UI recursion.
 * \author Xavier Lusetti
 */

#include "UIColors.hpp"
#include <fstream>
#include <string>

// Initialize our static map and pointer
std::unordered_map<wxString, Palette> UIColors::ThemeMap;
Palette* UIColors::Current = nullptr;

bool UIColors::LoadThemesFromFile(const wxString& filepath) {
    std::ifstream file(filepath.ToStdString());
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {

        // Clean up the line
        wxString themeName = wxString::FromUTF8(line.c_str());
        themeName.Trim(true).Trim(false);
        if (themeName.IsEmpty()) continue;


        Palette p;
        p.Name = themeName;


        // Helper lambda to read and trim the next line from the file
        auto readColor = [&]() -> wxColour {
            std::string hex;
            if (std::getline(file, hex)) {
                wxString wxHex = wxString::FromUTF8(hex.c_str());
                
                // Remove whitespace
                wxHex.Replace(" ", "");
                wxHex.Replace("\r", "");
                wxHex.Replace("\t", "");
                wxHex.Trim(true).Trim(false);

                if (wxHex.IsEmpty()) return *wxBLACK;
                
                wxColour col(wxHex);
                if (!col.IsOk()) {
                    wxLogError("Failed to parse color: '%s'", wxHex);
                    return *wxBLACK;
                }
                return col;
            }
            return *wxBLACK;
        };

        // Order must be matching within the .txt file
        p.Background    = readColor();
        p.Surface       = readColor();
        p.SurfaceRaised = readColor();
        p.Accent        = readColor();
        p.AccentHover   = readColor();
        p.TextPrimary   = readColor();
        p.TextSecondary = readColor();
        p.TextMuted     = readColor();
        p.Separator     = readColor();
        p.Danger        = readColor();

        ThemeMap[themeName] = p;
    }

    // Default to the first theme loaded if nothing is set
    if (ThemeMap.empty()) {
        SetTheme("Dark Mode");
    } else if (Current == nullptr) {
        Current = &ThemeMap.begin()->second;
    }

    return true;
}

void UIColors::SetTheme(const wxString& themeName) {
    auto it = ThemeMap.find(themeName);
    
    if (it != ThemeMap.end()) {
        Current = &it->second;
    } 
    // If it's not in the map, manually build the default
    else if (themeName == "Dark Mode") {
        Palette dark;
        dark.Name = "Dark Mode";
        dark.Background    = wxColour("#121212");
        dark.Surface       = wxColour("#1E1E1E");
        dark.SurfaceRaised = wxColour("#2C2C2C");
        dark.Accent        = wxColour("#8A00FF");
        dark.AccentHover   = wxColour("#A028FF");
        dark.TextPrimary   = wxColour("#FFFFFF");
        dark.TextSecondary = wxColour("#BDBDBD");
        dark.TextMuted     = wxColour("#757575");
        dark.Separator     = wxColour("#252525");
        dark.Danger        = wxColour("#FF4C29");

        ThemeMap["Dark Mode"] = dark;
        Current = &ThemeMap["Dark Mode"];
    }
}

ColorRole UIColors::GetRoleFromColour(const wxColour& col) {
    // Detect roles for current background and foreground by searching through the theme map
    for (auto const& [name, p] : ThemeMap) {
        if (col == p.Background)    return ColorRole::Background;
        if (col == p.Surface)       return ColorRole::Surface;
        if (col == p.SurfaceRaised) return ColorRole::SurfaceRaised;
        if (col == p.Accent)        return ColorRole::Accent;
        if (col == p.AccentHover)   return ColorRole::AccentHover;
        if (col == p.TextPrimary)   return ColorRole::TextPrimary;
        if (col == p.TextSecondary) return ColorRole::TextSecondary;
        if (col == p.TextMuted)     return ColorRole::TextMuted;
        if (col == p.Separator)     return ColorRole::Separator;
        if (col == p.Danger)        return ColorRole::Danger;
    }
    return ColorRole::None;
}

void UIColors::UpdateUI(wxWindow* win) {
    if (!win || !Current) return;

    ColorRole bgRole = GetRoleFromColour(win->GetBackgroundColour());
    ColorRole fgRole = GetRoleFromColour(win->GetForegroundColour());


    // Background
    switch (bgRole) {
        case ColorRole::Background:    win->SetBackgroundColour(Current->Background); break;
        case ColorRole::Surface:       win->SetBackgroundColour(Current->Surface); break;
        case ColorRole::SurfaceRaised: win->SetBackgroundColour(Current->SurfaceRaised); break;
        case ColorRole::Accent:        win->SetBackgroundColour(Current->Accent); break;
        case ColorRole::AccentHover:   win->SetBackgroundColour(Current->AccentHover); break;
        case ColorRole::Danger:        win->SetBackgroundColour(Current->Danger); break;
        case ColorRole::Separator:     win->SetBackgroundColour(Current->Separator); break;
        default: break; // Keep original color if no role matched
    }


    // Foreground
    switch (fgRole) {
        case ColorRole::TextPrimary:   win->SetForegroundColour(Current->TextPrimary); break;
        case ColorRole::TextSecondary: win->SetForegroundColour(Current->TextSecondary); break;
        case ColorRole::TextMuted:     win->SetForegroundColour(Current->TextMuted); break;
        case ColorRole::Accent:        win->SetForegroundColour(Current->Accent); break;
        case ColorRole::Danger:        win->SetForegroundColour(Current->Danger); break;
        default: break;
    }

    win->Refresh();
    win->Update(); 


    // Recurse through children
    for (auto* child : win->GetChildren()) {
        UpdateUI(child);
    }
}