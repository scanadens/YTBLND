/**
 * \file ButtonsConfig.hpp
 * \brief Centralized button size presets for UI layer widgets.
 */

/**
 * \defgroup UIButtonSizing Button Sizing Presets
 * \brief Centralized dimensions used by UI buttons.
 *
 * Presets are organized by usage area:
 * - Navigation: TopBarBack, Nav
 * - Forms: FormTab, FormSubmit
 * - Full-width actions: FullWidthPrimary, FullWidthSecondary, FullWidthDanger
 * - Utility/dialog actions: Compact, Small, Medium, Icon
 *
 * Use UIButtons::ApplySizeBounds() for direct control styling, or
 * UIButtons::GetSize() when a wxSize is required.
 *
 * Note: ApplySizeBounds() also applies an optional max size bound from config.
 *
 */

#pragma once

#include <wx/button.h>
#include <wx/gdicmn.h>

/**
 * \enum ButtonType
 * \brief Named presets used to keep button sizing consistent across panels.
 */
enum class ButtonType {
	TopBarBack = 0,    ///< Navigation back button (TopBar).
	Nav,               ///< Primary nav buttons used in page headers/home top rows.
	Compact,           ///< Small utility action (e.g., Add button).
	Small,             ///< Dialog action button size.
	Medium,            ///< Medium action button size.
	FormTab,           ///< Login/Register tab buttons.
	FormSubmit,        ///< Form submit actions (Sign In / Create Account).
	FullWidthPrimary,  ///< Full-width prominent action.
	FullWidthSecondary,///< Full-width secondary action.
	FullWidthDanger,   ///< Full-width destructive action (Log Out).
	Icon               ///< Square icon-style action (e.g., remove-row button).
};

/**
 * \struct BtnConfig
 * \brief Width/height dimensions for a button preset.
 *
 * A width of `-1` means the control can expand horizontally in an expanding
 * sizer while keeping the configured minimum height.
 */
struct BtnConfig {
	int minWidth;
	int minHeight;
	int maxWidth;
	int maxHeight;
};

namespace UIButtons {
	/**
	 * \brief Returns the raw dimension preset for a button type.
	 * \param type Preset key.
	 * \return Constant preset dimensions.
	 */
	const BtnConfig& GetConfig(ButtonType type);

	/**
	 * \brief Returns the configured minimum size for a preset.
	 * \param type Preset key.
	 * \return Minimum size constraints.
	 */
	wxSize GetMinSize(ButtonType type);

	/**
	 * \brief Returns the configured maximum size for a preset.
	 * \param type Preset key.
	 * \return Maximum size constraints. `(-1, -1)` means no max bound.
	 */
	wxSize GetMaxSize(ButtonType type);

	/**
	 * \brief Converts a preset into its minimum-size wxSize.
	 * \param type Preset key.
	 * \return Equivalent minimum-size wxSize for the selected preset.
	 */
	wxSize GetSize(ButtonType type);

	/**
	 * \brief Applies the selected preset's min and max size bounds.
	 * \param button Target button.
	 * \param type Preset key.
	 */
	void ApplySizeBounds(wxButton* button, ButtonType type);
}
