// ============================================================================
// UIPages.h — Page enum and NavigateFn type
//
// Defines every top-level page that lives in the MainFrame wxSimplebook and
// provides the NavigateFn callback type used by all sub-panels.
//
// HOW NAVIGATION WORKS
// ────────────────────
// MainFrame owns a wxSimplebook whose pages must be added in exactly the same
// order as the Page enum values (0, 1, 2, …).  Each sub-panel constructor
// receives a NavigateFn (a std::function<void(Page)>) that, when called,
// asks MainFrame to switch the visible page.  This way panels can trigger
// navigation without including MainFrame.h (which would create a circular
// dependency).
//
// ADDING A NEW PAGE
// ─────────────────
//  1. Add a new enumerator at the end of Page (keep the numbering
//     contiguous).
//  2. Create and add the corresponding wxPanel to the wxSimplebook inside
//     MainFrame::MainFrame() — in the same position as the enum value.
//  3. If the page needs reset/reload on arrival, add that call to
//     MainFrame::NavigateTo().
// ============================================================================

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
    SETTINGS           = 5,   // Settings (stub — back button only for now)
    BLEND_CHAT         = 6    // Chat tied to the active blend
};

// Callback type passed to every sub-panel.
// Call m_nav(Page::SOME_PAGE) to trigger a page switch in MainFrame.
using NavigateFn = std::function<void(Page)>;
