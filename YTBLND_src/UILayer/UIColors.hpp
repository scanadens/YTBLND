/**
 * \file UIColors.hpp
 * \brief Colour palletes to use throughout the GUI.
 * \author Xavier Lusetti
 * \author Jasmine Kumar
 *
 * All wxColour constants live here so the visual style
 * is consistent and easy to retheme.
 * 
 * Palette:
 * 
 * Name / Usage
 *      Background, Window / page background
 *      Surface / Card / panel backgrounds
 *      SurfaceRaised / Inputs, raised elements
 *      Accent / Buttons 
 *      AccentHover / Lighter purple for hover feedback
 *      TextPrimary / Body text
 *      TextSecondary / Secondary / caption text
 *      TextMuted / Placeholder / disabled text
 *      Separator / Rule lines
 *      Danger / Destructive-action buttons
 */

#pragma once

#include <wx/wx.h>
#include <wx/window.h>
#include <unordered_map>

enum class ColorRole {
    Background,
    Surface,
    SurfaceRaised,
    Accent,
    AccentHover,
    TextPrimary,
    TextSecondary,
    TextMuted,
    Separator,
    Danger,

    // If color doesn't match our palette return nothing
    None
};

/**
 * \struct Palette
 * \brief Palette class declaration.
 */
struct Palette {
    wxString Name; // The name of the colour theme associated with this palette.

    // Palette has attribute ^colour roles^
    wxColour Background;
    wxColour Surface;
    wxColour SurfaceRaised;
    wxColour Accent;
    wxColour AccentHover;
    wxColour TextPrimary;
    wxColour TextSecondary;
    wxColour TextMuted;
    wxColour Separator;
    wxColour Danger;
};

class UIColors {
public:
    // The dictionary storing all themes loaded from the text file (themes.txt).
    static std::unordered_map<wxString, Palette> ThemeMap;

    // An ordered list of theme string names
    static std::vector<wxString> ThemeOrder;

    // The current active palette pointer
    static Palette* Current;

    // Static Getter Methods used to get colours across the app.
    static wxColour Background()    { if (!Current) SetTheme("Dark Mode"); return Current->Background; }
    static wxColour Surface()       { if (!Current) SetTheme("Dark Mode"); return Current->Surface; }
    static wxColour SurfaceRaised() { if (!Current) SetTheme("Dark Mode"); return Current->SurfaceRaised; }
    static wxColour Accent()        { if (!Current) SetTheme("Dark Mode"); return Current->Accent; }
    static wxColour AccentHover()   { if (!Current) SetTheme("Dark Mode"); return Current->AccentHover; }
    static wxColour TextPrimary()   { if (!Current) SetTheme("Dark Mode"); return Current->TextPrimary; }
    static wxColour TextSecondary() { if (!Current) SetTheme("Dark Mode"); return Current->TextSecondary; }
    static wxColour TextMuted()     { if (!Current) SetTheme("Dark Mode"); return Current->TextMuted; }
    static wxColour Separator()     { if (!Current) SetTheme("Dark Mode"); return Current->Separator; }
    static wxColour Danger()        { if (!Current) SetTheme("Dark Mode"); return Current->Danger; }



    /**
     * \brief Parses the themes.txt file and populates the ThemeMap.
     * \param filepath The path to the themes.txt file.
     * \return True if successful, false if the file couldn't be loaded.
     */
    static bool LoadThemesFromFile(const wxString& filepath);

    /**
     * \brief Switches the active theme by looking up the name in the map.
     * \param themeName The string name of the theme (default: "Dark Mode").
     */
    static void SetTheme(const wxString& themeName);


    /**
     * \brief Updates the displaying UI with the active palette.
     * \param win The current window to be updated.
     */
    static void UpdateUI(wxWindow* win);

private:
    /**
     * \brief Checks a color against all loaded themes to find its role.
     * \param col The colour value being passed to check its role.
     * \return ColorRole The role matching the colour value.
     */
    static ColorRole GetRoleFromColour(const wxColour& col);
};