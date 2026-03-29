#pragma once

#include "File_ID.hpp"
#include "../ModelLayer/Video.hpp"

#include <list>
#include <optional>
#include <string>

/**
 * \file WatchHistoryParser.hpp
 * \author Shamar Pennant
 * \brief Specialized parser for YouTube watch-history HTML exports
 *
 * Reads a Google Takeout watch-history HTML file and extracts YouTube video IDs
 * from watched-video links.
 */
class WatchHistoryParser {
public:
    explicit WatchHistoryParser(const std::string& filePath);

    /**
     * Parses watch-history HTML and returns lightweight Video objects.
     * Only videoID is populated.
     */
    std::list<Video> parse();

    int getParserId() const;

private:
    std::string filePath;
    int parser_id;

    static std::optional<std::string> extractVideoIdFromHref(const std::string& href);
};