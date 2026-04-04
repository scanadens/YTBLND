/**
 * \file UIColors.hpp
 * \brief Shared dark-theme colour palette used throughout the UI.
 * \author Jasmine Kumar
 * \author Xavier Wah-Huen Lusetti
 *
 * All wxColour constants live here so the visual style is consistent and easy
 * to retheme.  Include this header instead of hard-coding colours in individual
 * panels.
 *
 * Palette (dark OLED-friendly theme):
 * | Name          | Hex       | Usage                              |
 * |---------------|-----------|------------------------------------|
 * | Background    | \#0D0D0D  | Window / page background           |
 * | Surface       | \#191919  | Card / panel backgrounds           |
 * | SurfaceRaised | \#262626  | Inputs, raised elements            |
 * | Accent        | \#8A00FF  | Brand purple - buttons, hover      |
 * | AccentHover   | \#A028FF  | Lighter purple for hover feedback  |
 * | TextPrimary   | \#FFFFFF  | Body text                          |
 * | TextSecondary | \#A0A0A0  | Secondary / caption text           |
 * | TextMuted     | \#646464  | Placeholder / disabled text        |
 * | Separator     | \#323232  | Rule lines                         |
 * | Danger        | \#DC3232  | Destructive-action buttons         |
 */

#pragma once
#include <wx/colour.h>

enum class ThemeType {
    Dark = 0,
    Light = 1,
    Neon = 2
};

/**
 * \struct Palette
 * \brief Palette class declaration.
 */
struct Palette {
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

/**
 * \brief Shared dark-theme colour constants for the YTBLND UI.
 */
namespace UIColors {
    extern Palette DarkMode;
    extern Palette LightMode;
    extern Palette NeonMode;
    extern Palette* Current;

    extern wxColour Background;
    extern wxColour Surface;
    extern wxColour SurfaceRaised;
    extern wxColour Accent;
    extern wxColour AccentHover;
    extern wxColour TextPrimary;
    extern wxColour TextSecondary;
    extern wxColour TextMuted;
    extern wxColour Separator;
    extern wxColour Danger;

    void SetTheme(ThemeType theme);

    /**
     * Returns the current ThemeType index.
     */
    ThemeType GetCurrentTheme();
}
