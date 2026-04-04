/**
 * \file IRefreshablePanel.hpp
 * \brief Interface for panels that support refresh operations.
 * \author Shamar Pennant
 *
 * IRefreshablePanel is a mixin interface that enables UI panels to receive
 * refresh notifications when they become visible or when data changes.
 * Implementing panels can update their display to reflect current application
 * state whenever Refresh() is called.
 */

#pragma once

/**
 * \brief Interface for panels that support refresh operations.
 */
/**
 * \class IRefreshablePanel
 * \brief IRefreshablePanel class declaration.
 */
class IRefreshablePanel {
public:
    virtual ~IRefreshablePanel() = default;
    
    /**
     * Refreshes the panel's display to reflect current application state.
     * Called by MainFrame whenever the panel becomes active.
     */
    virtual void Refresh() = 0;
};
