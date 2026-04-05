/**
 * \file ResourcePathUtils.cpp
 * \brief Shared resource path resolution helpers for UI components.
 */

#include "ResourcePathUtils.hpp"
#include "UIColors.hpp"

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace UIResourcePaths {

wxString FindResourcePath(const wxString& fileName)
{
    const wxArrayString relativeCandidates = {
        "YTBLND_src/resources/" + fileName,
        "resources/" + fileName,
        "../resources/" + fileName,
    };

    for (const auto& candidate : relativeCandidates) {
        if (wxFileExists(candidate)) {
            return candidate;
        }
    }

    wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
    const wxString exeDir = exePath.GetPath();
    const wxArrayString exeRelativeCandidates = {
        exeDir + "/../../YTBLND_src/resources/" + fileName,
        exeDir + "/../share/ytblnd/resources/" + fileName,
        exeDir + "/../resources/" + fileName,
        exeDir + "/resources/" + fileName,
    };

    for (const auto& candidate : exeRelativeCandidates) {
        if (wxFileExists(candidate)) {
            return candidate;
        }
    }

    return wxEmptyString;
}

wxString GetIconFolder()
{
    wxString themeName = "Dark Mode";
    if (UIColors::Current) {
        themeName = UIColors::Current->Name;
    }

    if (themeName == "Dark Mode")     return "dark";
    if (themeName == "Light Mode")    return "light";
    if (themeName == "Neon Mode")     return "neon";
    if (themeName == "Cyber Amber")   return "amber";
    if (themeName == "Forest Floor")  return "amber";
    if (themeName == "Nordic Frost")  return "light";
    // All remaining dark-background themes use dark icons
    return "dark";
}

}
