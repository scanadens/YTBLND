/**
 * \file BlendFeedPanel.hpp
 * \brief Home-page video feed panel that displays paged blend results.
 * \author Jasmine Kumar
 *
 * Displays six VideoCard widgets in a fixed 3x2 grid sourced from the active
 * blend in AppState.
 *
 * PAGING
 * ------
 * m_offset tracks which six-video slice of the blend is visible.
 * NextPage() advances by six entries (wrapping as needed), then refreshes via
 * LoadFromBlend().
 *
 * LOADING
 * -------
 * LoadFromBlend() reads AppState directly so it always reflects the current
 * active blend.
 */

#pragma once

#include <wx/wx.h>

#include "VideoCard.hpp"
#include "IRefreshablePanel.hpp"
#include "../AppLayer/AppController.hpp"

/**
 * \class BlendFeedPanel
 * \brief Panel that renders paged blend videos in a fixed 3x2 card layout.
 */
class BlendFeedPanel : public wxPanel, public IRefreshablePanel {
public:
    /**
     * \brief Construct a BlendFeedPanel.
     * \param parent Parent wxWidgets window.
     * \param controller Application controller used for refresh dispatches.
     */
    BlendFeedPanel(wxWindow* parent, AppController& controller);

    /**
     * \brief Refresh panel state from current application data.
     */
    void Refresh() override;

    /**
     * \brief Advance to the next six videos, wrapping around the blend.
     */
    void NextPage();

    /**
     * \brief Reload the current page from the active blend.
     *
     * Clears all cards if there is no active blend.
     */
    void LoadFromBlend();

private:
    static constexpr int kPageSize = 6;   // 3 cols x 2 rows

    AppController& m_controller;
    VideoCard*     m_cards[kPageSize];
    int            m_offset;   // index of the first card in the current page
};
