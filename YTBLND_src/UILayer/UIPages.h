#pragma once
#include <functional>

// All top-level pages in the MainFrame wxSimplebook.
// Each panel receives a NavigateFn so it can switch pages without
// knowing anything about MainFrame itself.
enum class Page {
    HOME           = 0,
    USER           = 1,
    SETTINGS       = 2,
    BLEND_CREATION = 3,
    BLEND_CHAT     = 4
};

using NavigateFn = std::function<void(Page)>;
