#pragma once
#include <functional>

// All top-level pages in the MainFrame wxSimplebook.
// Pages must be added in this exact numeric order in MainFrame::BuildPages().
// Each panel receives a NavigateFn so it can switch pages without
// knowing anything about MainFrame itself.
enum class Page {
    LOGIN              = 0,   // First screen — log in or register
    DATA_INSTRUCTIONS  = 1,   // Shown after login if user has no YouTube data
    BLEND_CREATION     = 2,   // Add participants and create a new blend
    HOME               = 3,   // Blend feed (title + 3x2 grid + refresh)
    USER               = 4,   // Account info + logout
    SETTINGS           = 5,   // Settings (stub)
    BLEND_CHAT         = 6    // Chat tied to the active blend
};

using NavigateFn = std::function<void(Page)>;
