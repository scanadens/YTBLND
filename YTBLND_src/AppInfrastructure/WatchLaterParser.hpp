#pragma once

#include "CsvSource.cpp"
#include "CsvParser.cpp"
#include "../ModelLayer/Video.h"

#include <list>
#include <string>
#include <unordered_map>

/**
 * \brief Parses a YouTube Takeout 'Watch later-videos.csv' into a list of Video
 *        objects with only the videoID field populated.
 *
 * Internally delegates file reading to CsvSource and parsing to CsvParser.
 * The resulting Videos have empty strings / zero for all fields except videoID,
 * because the Watch Later CSV only provides the ID and a timestamp.
 */
class WatchLaterParser {
public:
    explicit WatchLaterParser(const std::string& filePath) : filePath(filePath) {}

    /**
     * Reads and parses the Watch Later CSV.
     * \return list of Video objects, each with only videoID set.
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
