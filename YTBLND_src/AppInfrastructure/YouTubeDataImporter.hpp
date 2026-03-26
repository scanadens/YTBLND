#pragma once

#include "../ModelLayer/Video.h"

#include <list>
#include <string>

/**
 * \file YouTubeDataImporter.hpp
 * \author Shamar Pennant
 * \brief Entry-point importer that routes YouTube exports to the right parser
 *
 * Keeps AppController clean by hiding CSV-vs-HTML parser selection.
 */
class YouTubeDataImporter {
public:
    /**
     * Imports YouTube data from a supported file path.
     *
     * Supported extensions:
     * - .csv  : Watch Later export
     * - .html/.htm : watch-history export
     */
    std::list<Video> import(const std::string& filePath) const;

private:
    static std::string lowerExtension(const std::string& filePath);
};