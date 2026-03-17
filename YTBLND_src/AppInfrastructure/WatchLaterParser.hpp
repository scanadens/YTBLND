#pragma once

#include "CsvSource.cpp"
#include "CsvParser.cpp"
#include "../ModelLayer/Video.h"

#include <list>
#include <string>
#include <unordered_map>

/**
 * \file WatchLaterParser.hpp
 * \author Shamar Pennant
 * \brief Specialized parser for YouTube Takeout Watch Later CSV files
 * 
 * Parses YouTube Takeout 'Watch later-videos.csv' files into Video objects
 * with only the videoID field populated. Internally uses CsvSource for reading
 * and CsvParser for parsing. The resulting Videos have empty strings/zero for
 * all other fields since Watch Later CSV only provides ID and timestamp.
 */
/// Specialized parser for YouTube Takeout Watch Later CSV files.
class WatchLaterParser {
public:
    /** Constructs WatchLaterParser for the given CSV file.
     * \param filePath Path to the YouTube Takeout Watch Later CSV file
     */
    explicit WatchLaterParser(const std::string& filePath) : filePath(filePath) {}

    /** Reads and parses the Watch Later CSV file into Video objects.
     * 
     * Each returned Video object has only the videoID field populated;
     * all other fields (title, channelID, etc.) are empty/zero.
     * 
     * \return List of Video objects initialized from Watch Later CSV rows
     */
    std::list<Video> parse() {
        CsvSource source(filePath);
        CsvParser parser(source.read());

        std::list<std::unordered_map<std::string, std::string>> rows = parser.parse();
        std::list<Video> videos;

        for (const auto& row : rows) {
            auto it = row.find("Video ID");
            if (it != row.end() && !it->second.empty()) {
                videos.emplace_back(
                    it->second,          // videoID
                    "",                  // title  (unknown at this stage)
                    "",                  // channelID
                    "",                  // thumbnailURL
                    0,                   // duration
                    std::list<std::string>{}  // tags
                );
            }
        }

        return videos;
    }

private:
    std::string filePath;
};
