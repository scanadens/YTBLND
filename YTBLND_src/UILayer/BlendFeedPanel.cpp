// ============================================================================
// BlendFeedPanel.cpp — 3×2 video grid for the Home page implementation
// ============================================================================

#include "BlendFeedPanel.h"

#include <wx/sizer.h>

#include "UIColors.h"
#include "../AppLayer/AppState.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
BlendFeedPanel::BlendFeedPanel(wxWindow* parent, AppController& controller)
    : wxPanel(parent, wxID_ANY)
    , m_controller(controller)
    , m_offset(0)
{
    SetBackgroundColour(UIColors::Background);

    // 2 rows × 3 cols, 16 px gaps in both directions
    wxGridSizer* grid = new wxGridSizer(2, 3, 16, 16);

    for (int i = 0; i < kPageSize; ++i) {
        m_cards[i] = new VideoCard(this, m_controller);
        grid->Add(m_cards[i], 0, wxEXPAND);
    }

    // Outer sizer adds a uniform margin around the grid
    wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
    outer->Add(grid, 1, wxEXPAND | wxALL, 16);
    SetSizer(outer);

    LoadFromBlend();
}

// ---------------------------------------------------------------------------
// NextPage
// ---------------------------------------------------------------------------
void BlendFeedPanel::NextPage()
{
    Blend* blend = AppState::getInstance().getActiveBlend();
    if (!blend || blend->size() == 0) {
        // Nothing to page through — just stay at 0 and show placeholders
        m_offset = 0;
        LoadFromBlend();
        return;
    }

    m_offset = (m_offset + kPageSize) % blend->size();
    LoadFromBlend();
}

// ---------------------------------------------------------------------------
// LoadFromBlend
// ---------------------------------------------------------------------------
void BlendFeedPanel::LoadFromBlend()
{
    Blend* blend = AppState::getInstance().getActiveBlend();

    if (!blend || blend->size() == 0) {
        // No active blend — show all cards in placeholder state
        for (int i = 0; i < kPageSize; ++i) {
            m_cards[i]->Clear();
        }
        return;
    }

    const int total = blend->size();

    // Clamp offset in case the blend shrank since last NextPage()
    if (m_offset >= total) {
        m_offset = 0;
    }

    for (int i = 0; i < kPageSize; ++i) {
        int idx = (m_offset + i) % total;
        m_cards[i]->SetVideo(blend->getVideo(idx));
    }
}
