/**
 * \file UIPages.hpp
 * \brief Page enum and NavigateFn callback type for the MainFrame page switcher.
 *  \author Jasmine Kumar
 *
 * Defines every top-level page hosted in the MainFrame wxSimplebook and the
 * NavigateFn callback type passed to sub-panels for page switching.
 *
 * Pages must be added to the wxSimplebook in the same order as the Page enum
 * values (0, 1, 2, …).  Sub-panels call \c m_nav(Page::TARGET) to navigate
 * without depending on MainFrame directly.
 *
 * ### Adding a new page
 * 1. Add a new enumerator at the end of \c Page (keep values contiguous).
 * 2. Create and add the corresponding wxPanel in MainFrame::BuildPages() at
 *    the matching position.
 * 3. If the page needs reset/reload on arrival, add the call in
 *    MainFrame::NavigateTo().
 */

#pragma once
#include <functional>

/**
 * \brief All top-level pages in the MainFrame wxSimplebook.
 *
 * Numeric values must match the order panels are added in MainFrame::BuildPages().
 */
enum class Page {
    LOGIN             = 0,  ///< Sign-in / register screen.
    DATA_INSTRUCTIONS = 1,  ///< Shown after login when the user has no YouTube data.
    BLEND_CREATION    = 2,  ///< Add participants and create a new blend.
    HOME              = 3,  ///< Blend feed (3×2 grid + refresh).
    USER              = 4,  ///< Account info and logout.
    SETTINGS          = 5,  ///< Settings (stub — back button only).
    BLEND_CHAT        = 6   ///< Chat tied to the active blend.
};

/**
 * \brief Callback type passed to every sub-panel for initiating page transitions.
 *
 * Call \c m_nav(Page::SOME_PAGE) to trigger a page switch in MainFrame.
 */
using NavigateFn = std::function<void(Page)>;
