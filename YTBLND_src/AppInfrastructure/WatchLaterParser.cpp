/**
 * \file WatchLaterParser.cpp
 * \brief Implementation for WatchLaterParser.
 * \author Shamar Pennant
 * \author Jasmine Kumar
 */

#include "WatchLaterParser.hpp"

#include <memory>
#include <fstream>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <iostream>

WatchLaterParser::WatchLaterParser(const std::string& filePath) : filePath(filePath) {
	parser_id = File_ID::CSV;
}

std::list<Video> WatchLaterParser::parse() {
    DataExtractor extractor(
        std::make_unique<CsvSource>(filePath),
        std::make_unique<CsvParser>()
    );

    auto rows = extractor.extract();
    std::list<Video> videos;

    for (const auto& row : rows) {
        auto it = row.find("Video ID");
        if (it != row.end() && !it->second.empty()) {
            videos.emplace_back(
                it->second,
                "",
                "",
                "",
                0,
                std::list<std::string>{}
            );
        }
    }

    return videos;
}

int WatchLaterParser::getParserId() {return parser_id;}

std::list<Video> WatchLaterParser::parseMultiThreaded(std::function<void(double)> progressCallback) {
    // Multi-threaded CSV parsing: spawn 3 worker threads to process rows in parallel.
    // Each thread processes a range of rows to maximize throughput.

    std::ifstream input(filePath);
    if (!input.is_open()) {
        throw std::runtime_error("WatchLaterParser: unable to open file " + filePath);
    }

    if (progressCallback) progressCallback(0.1);

    // Read all lines into memory for parallel processing.
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    input.close();

    if (lines.empty()) {
        if (progressCallback) progressCallback(1.0);
        return std::list<Video>();
    }

    if (progressCallback) progressCallback(0.2);

    // First line is typically the header; process starting from line 1.
    std::size_t dataStartIdx = (lines.size() > 0 && lines[0].find("Video ID") != std::string::npos) ? 1 : 0;
    std::size_t totalRows = lines.size() - dataStartIdx;

    if (totalRows == 0) {
        if (progressCallback) progressCallback(1.0);
        return std::list<Video>();
    }

    // Divide rows into 3 sections for parallel processing.
    const std::size_t section1Start = dataStartIdx;
    const std::size_t section1End = dataStartIdx + (totalRows / 3);
    const std::size_t section2Start = section1End;
    const std::size_t section2End = dataStartIdx + (2 * totalRows / 3);
    const std::size_t section3Start = section2End;
    const std::size_t section3End = lines.size();

    std::list<Video> videos1, videos2, videos3;
    std::vector<std::thread> threads;

    auto parseRowsFunc = [&lines](std::size_t startIdx, std::size_t endIdx) -> std::list<Video> {
        // Each thread parses its range of CSV rows independently.
        // Extracts the Video ID (first column, comma-separated).
        std::list<Video> result;
        if (lines.empty() || startIdx >= lines.size()) {
            return result;
        }

        for (std::size_t i = startIdx; i < endIdx && i < lines.size(); ++i) {
            const std::string& row = lines[i];
            if (row.empty()) {
                continue;
            }

            // CSV format: "Video ID, Added on ..."
            std::size_t commaPos = row.find(',');
            std::string videoID = (commaPos != std::string::npos) ? row.substr(0, commaPos) : row;

            // Trim quotes if present.
            if (!videoID.empty() && videoID.front() == '"' && videoID.back() == '"') {
                videoID = videoID.substr(1, videoID.size() - 2);
            }

            if (!videoID.empty()) {
                result.emplace_back(videoID, "", "", "", 0, std::list<std::string>{});
            }
        }
        return result;
    };

    // Spawn 3 threads.
    threads.emplace_back([&]() { videos1 = parseRowsFunc(section1Start, section1End); });
    threads.emplace_back([&]() { videos2 = parseRowsFunc(section2Start, section2End); });
    threads.emplace_back([&]() { videos3 = parseRowsFunc(section3Start, section3End); });

    if (progressCallback) progressCallback(0.5);

    // Wait for all threads.
    for (auto& thread : threads) {
        thread.join();
    }

    if (progressCallback) progressCallback(0.8);

    // Merge results, deduplicating by video ID.
    std::unordered_set<std::string> seenVideoIds;
    std::list<Video> mergedVideos;

    for (const auto& video : videos1) {
        if (seenVideoIds.insert(video.getVideoID()).second) {
            mergedVideos.push_back(video);
        }
    }
    for (const auto& video : videos2) {
        if (seenVideoIds.insert(video.getVideoID()).second) {
            mergedVideos.push_back(video);
        }
    }
    for (const auto& video : videos3) {
        if (seenVideoIds.insert(video.getVideoID()).second) {
            mergedVideos.push_back(video);
        }
    }

    if (progressCallback) progressCallback(1.0);

    std::cout << "[WatchLaterParser] Multi-threaded parse complete: \""
              << filePath << "\" extracted " << mergedVideos.size() << " unique videos\n";

    return mergedVideos;
}
