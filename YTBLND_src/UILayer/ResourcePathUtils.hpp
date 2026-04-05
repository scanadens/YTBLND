/**
 * \file ResourcePathUtils.hpp
 * \author Shamar Pennant
 * \brief Shared resource path resolution helpers for UI components.
 */

#pragma once

#include <wx/string.h>

namespace UIResourcePaths {

/**
 * \brief Resolve a UI resource path for both source-tree and installed layouts.
 * \param fileName Resource file path relative to the resources root.
 * \return Path to the first matching resource, or empty string if not found.
 */
wxString FindResourcePath(const wxString& fileName);

/**
 * \brief Returns the icon folder name for the current theme.
 *
 * Maps theme names to icon folder names. Themes without their own icon set
 * fall back to the closest visual match (dark or light).
 */
wxString GetIconFolder();

}
