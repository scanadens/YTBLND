#pragma once

#include <wx/wx.h>

#include "VideoCard.h"
#include "../AppLayer/AppController.h"

// ---------------------------------------------------------------------------
// BlendFeedPanel
//
// Displays 6 VideoCards in a 3-column × 2-row grid.
// Call LoadFromBlend() to (re)populate the cards from AppState::getActiveBlend().
// Call NextPage()     to advance to the next 6 videos (wraps around).
// ---------------------------------------------------------------------------
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
