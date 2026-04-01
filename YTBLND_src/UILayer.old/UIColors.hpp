/**
 * \file UIColors.hpp
 * \brief Shared dark-theme colour palette used throughout the UI.
 *  \author Jasmine Kumar
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
 * | Accent        | \#8A00FF  | Brand purple — buttons, hover      |
 * | AccentHover   | \#A028FF  | Lighter purple for hover feedback  |
 * | TextPrimary   | \#FFFFFF  | Body text                          |
 * | TextSecondary | \#A0A0A0  | Secondary / caption text           |
 * | TextMuted     | \#646464  | Placeholder / disabled text        |
 * | Separator     | \#323232  | Rule lines                         |
 * | Danger        | \#DC3232  | Destructive-action buttons         |
 */

#pragma once
#include <wx/colour.h>

/// Shared dark-theme colour constants for the YTBLND UI.
namespace UIColors {
    inline const wxColour Background   (13,  13,  13);   ///< \#0D0D0D — window background.
    inline const wxColour Surface      (25,  25,  25);   ///< \#191919 — panel / card background.
    inline const wxColour SurfaceRaised(38,  38,  38);   ///< \#262626 — inputs, raised elements.

    inline const wxColour Accent       (138,  0, 255);   ///< \#8A00FF — brand purple.
    inline const wxColour AccentHover  (160, 40, 255);   ///< \#A028FF — lighter hover purple.

    inline const wxColour TextPrimary  (255, 255, 255);  ///< White body text.
    inline const wxColour TextSecondary(160, 160, 160);  ///< Grey secondary text.
    inline const wxColour TextMuted    (100, 100, 100);  ///< Dark grey placeholder text.

    inline const wxColour Separator    (50,  50,  50);   ///< Rule-line colour.
    inline const wxColour Danger       (220,  50,  50);  ///< Red destructive-action colour.
}
