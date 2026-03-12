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
