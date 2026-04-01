/**
 * \file ButtonsConfig.cpp
 * \brief Implements shared button sizing presets used across UILayer.
 * \author Shamar Pennant
 */

#include "ButtonsConfig.hpp"

namespace {
/**
 * Internal constants for each ButtonType preset.
 * These are intentionally kept in one place to avoid size drift across panels.
 */
constexpr BtnConfig kTopBarBack = {80, 30, 180, 36};
constexpr BtnConfig kNav = {96, 36, 220, 44};
constexpr BtnConfig kCompact = {80, 34, 160, 40};
constexpr BtnConfig kSmall = {100, 34, 180, 40};
constexpr BtnConfig kMedium = {120, 34, 260, 42};
constexpr BtnConfig kLarge = {140, 34, 280, 42};
constexpr BtnConfig kFormTab = {120, 36, 260, 44};
constexpr BtnConfig kFormSubmit = {140, 40, 320, 48};
constexpr BtnConfig kFullWidthPrimary = {160, 40, 560, 48};
constexpr BtnConfig kFullWidthSecondary = {160, 32, 560, 40};
constexpr BtnConfig kFullWidthDanger = {160, 40, 560, 48};
constexpr BtnConfig kIcon = {30, 30, 44, 44};
}

namespace UIButtons {

const BtnConfig& GetConfig(ButtonType type)
{
    switch (type) {
    case ButtonType::TopBarBack:         return kTopBarBack;
    case ButtonType::Nav:                return kNav;
    case ButtonType::Compact:            return kCompact;
    case ButtonType::Small:              return kSmall;
    case ButtonType::Medium:             return kMedium;
	case ButtonType::Large: 			 return kLarge;
    case ButtonType::FormTab:            return kFormTab;
    case ButtonType::FormSubmit:         return kFormSubmit;
    case ButtonType::FullWidthPrimary:   return kFullWidthPrimary;
    case ButtonType::FullWidthSecondary: return kFullWidthSecondary;
    case ButtonType::FullWidthDanger:    return kFullWidthDanger;
    case ButtonType::Icon:               return kIcon;
    }

    return kNav;
}

wxSize GetSize(ButtonType type)
{
    return GetMinSize(type);
}

wxSize GetMinSize(ButtonType type)
{
    const BtnConfig& cfg = GetConfig(type);
    return wxSize(cfg.minWidth, cfg.minHeight);
}

wxSize GetMaxSize(ButtonType type)
{
    const BtnConfig& cfg = GetConfig(type);
    return wxSize(cfg.maxWidth, cfg.maxHeight);
}

void ApplySizeBounds(wxButton* button, ButtonType type)
{
    if (!button) {
        return;
    }

    button->SetMinSize(GetMinSize(type));
    button->SetMaxSize(GetMaxSize(type));
}

} // namespace UIButtons
