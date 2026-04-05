/**
 * \file UIColors.cpp
 * \brief Logic for dynamic theme switching and UI recursion.
 * \author Xavier Lusetti
 */

#include "UIColors.hpp"
#include "ResourcePathUtils.hpp"

#include <fstream>
#include <string>
#include <algorithm>

#include <wx/bmpbndl.h>
#include <wx/mstream.h>
#include <wx/ffile.h>

// Initialize;
// unordered map of palettes
// vector of ordered themes 
// and pointer to the current palette
std::unordered_map<wxString, Palette> UIColors::ThemeMap;
std::vector<wxString> UIColors::ThemeOrder;
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

        // Helper to read and trim the next line from the file
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

        // Record the string name in order IF it isn't already there
        if (std::find(ThemeOrder.begin(), ThemeOrder.end(), themeName) == ThemeOrder.end()) {
            ThemeOrder.push_back(themeName);
        }
    }

    // DEFAULT to the first theme loaded if nothing is set
    if (ThemeMap.empty()) {
        SetTheme("Dark Mode");
    // SUCCESSFUL loading. Use the first loaded theme
    } else if (Current == nullptr) {
        Current = &ThemeMap[ThemeOrder[0]];
    }

    return true;
}

void UIColors::SetTheme(const wxString& themeName) {
    auto it = ThemeMap.find(themeName);
    
    if (it != ThemeMap.end()) {
        Current = &it->second;
    } 
    // DEFAULT fail safe
    else if (themeName == "Dark Mode") {
        // Record the details, update the map, update the pointer, update the ordered vector
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
        if (std::find(ThemeOrder.begin(), ThemeOrder.end(), "Dark Mode") == ThemeOrder.end()) {
            ThemeOrder.push_back("Dark Mode");
        }
    }
}

wxButton* UIColors::MakeIconButton(wxWindow* parent, const wxString& iconName, 
                                   ColorRole bgRole, int iconSize) 
{
    // Create the button with a size slightly larger than the icon for padding
    wxButton* btn = new wxButton(parent, wxID_ANY, wxEmptyString, 
                                 wxDefaultPosition, wxSize(iconSize + 12, iconSize + 12), 
                                 wxBORDER_NONE);

    // Tag the button with the icon name so OnThemeUpdate knows which SVG to reload
    btn->SetName(iconName);

    // Apply initial theme colors
    wxColour bg;
    switch(bgRole) {
        case ColorRole::Surface: bg = Surface(); break;
        default:                 bg = Background(); break;
    }
    btn->SetBackgroundColour(bg);

    // Load and set the initial SVG
    wxBitmapBundle icon = GetIcon(iconName + ".svg", ColorRole::TextPrimary, iconSize);
    if (icon.IsOk()) {
        btn->SetBitmap(icon);
    }

    return btn;
}

wxBitmapBundle UIColors::GetIcon(const wxString& iconName, ColorRole role, int size) {
    wxString folder = UIResourcePaths::GetIconFolder();
    wxString path = UIResourcePaths::FindResourcePath("icons/" + folder + "/" + iconName);
    
    if (path.empty() || !Current) return wxBitmapBundle();

    wxFFile file(path, "r");
    if (!file.IsOpened()) return wxBitmapBundle();

    wxString svgContent;
    file.ReadAll(&svgContent);

    // Map the role to the current palette's color
    wxColour targetColor;
    switch(role) {
        case ColorRole::Background:    targetColor = Current->Background;       break;
        case ColorRole::Surface:       targetColor = Current->Surface;          break;
        case ColorRole::SurfaceRaised: targetColor = Current->SurfaceRaised;    break;
        case ColorRole::Accent:        targetColor = Current->Accent;           break;
        case ColorRole::AccentHover:   targetColor = Current->AccentHover;      break;
        case ColorRole::TextPrimary:   targetColor = Current->TextPrimary;      break;
        case ColorRole::TextSecondary: targetColor = Current->TextPrimary;      break;
        case ColorRole::TextMuted:     targetColor = Current->TextPrimary;      break;
        case ColorRole::Separator:     targetColor = Current->Separator;        break;
        case ColorRole::Danger:        targetColor = Current->Danger;           break;
        default:                       targetColor = Current->TextPrimary;
    }

    svgContent.Replace("CURRENT_COLOR", targetColor.GetAsString(wxC2S_HTML_SYNTAX));

    return wxBitmapBundle::FromSVG(svgContent, wxSize(size, size));
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

    // Generic check for themed elements (.svg elements)
    if (auto* themed = dynamic_cast<IThemedElement*>(win)) {
        themed->OnThemeUpdate();
    }

    // Recurse through children
    for (auto* child : win->GetChildren()) {
        UpdateUI(child);
    }
}