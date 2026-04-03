#pragma once

#include "File_ID.hpp"
#include "../ModelLayer/Video.hpp"

#include <list>
#include <optional>
#include <string>
#include <vector>
#include <functional>

/**
 * \file WatchHistoryParser.hpp
 * \author Shamar Pennant
 * \brief Specialized parser for YouTube watch-history HTML exports
 *
 * Reads a Google Takeout watch-history HTML file and extracts YouTube video IDs
 * from watched-video links. Supports both single-threaded and multi-threaded parsing.
 */
class WatchHistoryParser {
public:
    explicit WatchHistoryParser(const std::string& filePath);

    /**
     * Parses watch-history HTML and returns lightweight Video objects.
     * Only videoID is populated.
     */
    std::list<Video> parse();

    /**
     * Multi-threaded parse using 2-3 worker threads for faster processing.
     * Each thread processes a section of the file to avoid duplicate work.
     * \param progressCallback Optional callback to report parsing progress (0.0 to 1.0)
     * \return List of unique Video objects extracted from all threads
     */
    std::list<Video> parseMultiThreaded(std::function<void(double)> progressCallback = nullptr);

    int getParserId() const;

private:
    std::string filePath;
    int parser_id;

    static std::optional<std::string> extractVideoIdFromHref(const std::string& href);
    
    /**
     * Parse a range of HTML content.
     * \param htmlContent Full HTML file content
     * \param startPos Starting character position for this thread
     * \param endPos Ending character position for this thread
     * \return List of Video objects found in this section
     */
    static std::list<Video> parseRange(const std::string& htmlContent, std::size_t startPos, std::size_t endPos);
};