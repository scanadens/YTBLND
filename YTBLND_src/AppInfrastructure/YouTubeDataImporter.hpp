#pragma once

#include "../ModelLayer/Video.hpp"

#include <list>
#include <string>
#include <functional>

/**
 * \file YouTubeDataImporter.hpp
 * \author Shamar Pennant
 * \brief Entry-point importer that routes YouTube exports to the right parser
 *
 * Keeps AppController clean by hiding CSV-vs-HTML parser selection.
 * Supports both single-threaded and multi-threaded import with progress tracking.
 */
/**
 * \class YouTubeDataImporter
 * \brief YouTubeDataImporter class declaration.
 */
class YouTubeDataImporter {
public:
    /**
     * Imports YouTube data from a supported file path.
     *
     * Supported extensions:
     * - .csv  : Watch Later export
     * - .html/.htm : watch-history export
     * 
     * \param filePath path to the specified file
     * \return \code list<video> \endcode extracted and formatted from \code filePath \endcode
     */
    std::list<Video> import(const std::string& filePath) const;

    /**
     * Multi-threaded import with progress feedback.
     * Uses 3 worker threads to process the file faster.
     * \param filePath Path to YouTube export file
     * \param progressCallback Optional callback receiving progress 0.0-1.0
     * \return List of Video objects extracted from file (ID-only)
     */
    std::list<Video> importMultiThreaded(const std::string& filePath,
                                         std::function<void(double)> progressCallback = nullptr) const;

private:
    /**
     * Helper to ensure extensions are always lowered
     * \param filePath desired file
     * \return \code string \endcode of the file path with the lowered extension
     */
    static std::string lowerExtension(const std::string& filePath);
};
