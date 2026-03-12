// ============================================================================
// UIColors.h — Shared dark-theme colour palette
//
// All wxColour constants used across the UI live here so the look is
// consistent and easy to retheme.  Include this header instead of
// hard-coding colours in individual panels.
//
// Palette overview (dark OLED-friendly theme):
//   Background    #0D0D0D  window / page background
//   Surface       #191919  card / panel backgrounds
//   SurfaceRaised #262626  inputs, raised elements, inactive tabs
//   Accent        #8A00FF  brand purple — buttons, active tab, hover border
//   AccentHover   #A028FF  lighter purple for hover feedback on Accent widgets
//   TextPrimary   #FFFFFF  body text
//   TextSecondary #A0A0A0  secondary / caption text
//   TextMuted     #646464  placeholder / disabled text
//   Separator     #323232  horizontal/vertical rule lines
//   Danger        #DC3232  destructive-action buttons (logout, delete)
//
// TODO: If a light-mode variant is ever needed, replace inline constants
//       with a theme object loaded at runtime.
// ============================================================================

#pragma once
#include <wx/colour.h>

namespace UIColors {
    // Backgrounds
    inline const wxColour Background   (13,  13,  13);   // #0D0D0D  — window bg
    inline const wxColour Surface      (25,  25,  25);   // #191919  — panel bg
    inline const wxColour SurfaceRaised(38,  38,  38);   // #262626  — cards, inputs

    // Accent
    inline const wxColour Accent       (138,  0, 255);   // #8A00FF  — purple brand
    inline const wxColour AccentHover  (160, 40, 255);   // #A028FF

    // Text
    inline const wxColour TextPrimary  (255, 255, 255);
    inline const wxColour TextSecondary(160, 160, 160);
    inline const wxColour TextMuted    (100, 100, 100);

    // Utility
    inline const wxColour Separator    (50,  50,  50);
    inline const wxColour Danger       (220,  50,  50);  // error / destructive
}
