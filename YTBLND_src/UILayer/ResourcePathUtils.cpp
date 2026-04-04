/**
 * \file ResourcePathUtils.cpp
 * \brief Shared resource path resolution helpers for UI components.
 */

#include "ResourcePathUtils.hpp"

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

}
