/**
 * \file ActiveBlendsPanel.hpp
 * \brief Panel for viewing and managing active blends.
 * \author Jasmine Kumar
 */

#pragma once

#include <wx/wx.h>
#include <wx/scrolwin.h>

#include "IRefreshablePanel.hpp"
#include "UIPages.hpp"

class AppController;

/**
 * \class ActiveBlendsPanel
 * \brief ActiveBlendsPanel class declaration.
 */
class ActiveBlendsPanel : public wxPanel, public IRefreshablePanel {
public:
    ActiveBlendsPanel(wxWindow* parent, AppController& controller, NavigateFn nav);
    void Refresh() override;

private:
    AppController& m_controller;
    NavigateFn     m_nav;

    wxScrolledWindow* m_listScroll;
    wxPanel*          m_listInner;
    wxBoxSizer*       m_listSizer;
    wxStaticText*     m_emptyLabel;

    void RebuildList();
    void OnSelectBlend(const std::string& blendID, const std::string& title);
    void OnLeaveBlend(const std::string& blendID, const std::string& title);
};
