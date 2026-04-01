#pragma once

class IRefreshablePanel {
public:
    virtual ~IRefreshablePanel() = default;
    virtual void Refresh() = 0;
};
