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
    static std::string lowerExtension(const std::string& filePath);
};