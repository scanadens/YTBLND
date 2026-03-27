// ============================================================================
// BlendFeedPanel.h — 3×2 video grid for the Home page
//
// Displays 6 VideoCards in a fixed 3-column × 2-row grid.  Each card shows
// one video from the active blend (AppState::getActiveBlend()).
//
// PAGING
// ──────
// m_offset tracks which 6-video slice of the blend is currently shown.
// NextPage() advances the offset by 6 (wrapping around), then calls
// LoadFromBlend() to refresh the cards.  The Refresh button on the home
// page calls MainFrame::TriggerFeedRefresh() → NextPage().
//
// LOADING
// ───────
// LoadFromBlend() reads AppState directly (no parameter) so it always gets
// the latest blend.  Call it after a blend is created/loaded to show the
// first page, or from NextPage() to advance.
//
// TODO: Subscribe to an AppState "blendChanged" event so the feed auto-
//       refreshes when a new blend becomes active, without requiring a
//       manual Refresh button press.
// TODO: Add animation / transition when paging between sets of videos.
// ============================================================================

#pragma once

#include <wx/wx.h>

#include "VideoCard.h"
#include "../AppLayer/AppController.hpp"

class BlendFeedPanel : public wxPanel {
public:
    BlendFeedPanel(wxWindow* parent, AppController& controller);

    // Advance to the next 6 videos, wrapping around the blend.
    void NextPage();

    // (Re)load the current page from the active blend.
    // Clears all cards if there is no active blend.
    void LoadFromBlend();

private:
    static constexpr int kPageSize = 6;   // 3 cols × 2 rows

    AppController& m_controller;
    VideoCard*     m_cards[kPageSize];
    int            m_offset;   // index of the first card in the current page
};
