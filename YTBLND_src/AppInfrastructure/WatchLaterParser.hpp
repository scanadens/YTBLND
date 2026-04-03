#pragma once

#include "DataExtractor.hpp"
#include "CsvSource.hpp"
#include "CsvParser.hpp"
#include "../ModelLayer/Video.hpp"

#include <list>
#include <string>
#include <functional>

/**
 * \file WatchLaterParser.hpp
 * \author Shamar Pennant
 * \brief Specialized parser for YouTube Takeout Watch Later CSV files
 * 
 * Parses YouTube Takeout 'Watch later-videos.csv' files into Video objects
 * with only the videoID field populated. Internally uses CsvSource for reading
 * and CsvParser for parsing. The resulting Videos have empty strings/zero for
 * all other field since Watch Later CSV only provides ID and timestamp.
 * Supports both single-threaded and multi-threaded parsing.
 */

/// Specialized parser for YouTube Takeout Watch Later CSV files.
class WatchLaterParser {
public:
    /** Constructs WatchLaterParser for the given CSV file.
     * \param filePath Path to the YouTube Takeout Watch Later CSV file.
     * Also ensures that the CSV parser id is assigned
     */
    explicit WatchLaterParser(const std::string& filePath);

    /** Reads and parses the Watch Later CSV file into Video objects.
     * 
     * Each returned Video object has only the videoID field populated;
     * all other fields (title, channelID, etc.) are empty/zero.
     * 
     * \return List of Video objects initialized from Watch Later CSV rows
     */
    std::list<Video> parse();

    /**
     * Multi-threaded parse using 2-3 worker threads for faster CSV processing.
     * Each thread processes a range of CSV rows to avoid duplicate work.
     * \param progressCallback Optional callback to report parsing progress (0.0 to 1.0)
     * \return List of Video objects extracted from all threads
     */
    std::list<Video> parseMultiThreaded(std::function<void(double)> progressCallback = nullptr);

    int getParserId();

private:
    std::string filePath;
    int parser_id;
};
