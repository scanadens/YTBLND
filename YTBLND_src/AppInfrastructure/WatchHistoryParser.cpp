/**
 * \file WatchHistoryParser.cpp
 * \brief Implementation for WatchHistoryParser.
 * \author Shamar Pennant
 */

#include "WatchHistoryParser.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <iostream>

WatchHistoryParser::WatchHistoryParser(const std::string& filePath) : filePath(filePath) {
    parser_id = File_ID::HTML;
}

std::optional<std::string> WatchHistoryParser::extractVideoIdFromHref(const std::string& href) {
    std::string normalized = href;
    for (char& c : normalized) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    std::size_t watchPos = normalized.find("youtube.com/watch");
    if (watchPos != std::string::npos) {
        std::size_t queryPos = href.find('?', watchPos);
        if (queryPos == std::string::npos) {
            return std::nullopt;
        }

        std::string query = href.substr(queryPos + 1);
        std::size_t start = 0;
        while (start < query.size()) {
            std::size_t amp = query.find('&', start);
            std::string token = query.substr(start, amp == std::string::npos ? std::string::npos : amp - start);

            if (token.rfind("v=", 0) == 0 && token.size() > 2) {
                return token.substr(2);
            }

            if (amp == std::string::npos) {
                break;
            }
            start = amp + 1;
        }
    }

    std::size_t shortPos = normalized.find("youtu.be/");
    if (shortPos != std::string::npos) {
        std::size_t idStart = shortPos + std::string("youtu.be/").size();
        std::size_t idEnd = href.find_first_of("?&#\"'", idStart);
        std::string id = href.substr(idStart, idEnd == std::string::npos ? std::string::npos : idEnd - idStart);
        if (!id.empty()) {
            return id;
        }
    }

    return std::nullopt;
}

std::list<Video> WatchHistoryParser::parse() {
    if (filePath.empty()) {
        throw std::invalid_argument("WatchHistoryParser: file path is empty");
    }

    std::ifstream input(filePath, std::ios::in);
    if (!input.is_open()) {
        throw std::runtime_error("WatchHistoryParser: unable to open file " + filePath);
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string html = buffer.str();

    std::list<Video> videos;
    std::unordered_set<std::string> seenVideoIds;

    // Lightweight anchor scan to avoid hard dependency on external HTML parsers.
    std::size_t searchPos = 0;
    while (true) {
        std::size_t hrefPos = html.find("href=\"", searchPos);
        if (hrefPos == std::string::npos) {
            break;
        }

        std::size_t valueStart = hrefPos + 6;
        std::size_t valueEnd = html.find('"', valueStart);
        if (valueEnd == std::string::npos) {
            break;
        }

        std::string href = html.substr(valueStart, valueEnd - valueStart);
        auto videoId = extractVideoIdFromHref(href);

        if (videoId.has_value() && seenVideoIds.insert(*videoId).second) {
            videos.emplace_back(*videoId, "", "", "", 0, std::list<std::string>{});
        }

        searchPos = valueEnd + 1;
    }

    return videos;
}

int WatchHistoryParser::getParserId() const {
    return parser_id;
}

std::list<Video> WatchHistoryParser::parseMultiThreaded(std::function<void(double)> progressCallback) {
    // Multi-threaded parsing: spawn 3 worker threads to process the file in parallel.
    // Each thread processes a distinct section to avoid duplicate work and improve
    // throughput on large HTML exports (e.g., 30K+ video IDs).

    if (filePath.empty()) {
        throw std::invalid_argument("WatchHistoryParser: file path is empty");
    }

    std::ifstream input(filePath, std::ios::in | std::ios::binary | std::ios::ate);
    if (!input.is_open()) {
        throw std::runtime_error("WatchHistoryParser: unable to open file " + filePath);
    }

    // Read entire file at once for consistent parsing across threads.
    std::size_t fileSize = input.tellg();
    input.seekg(0);
    
    std::string htmlContent(fileSize, '\0');
    input.read(&htmlContent[0], fileSize);
    input.close();

    if (progressCallback) progressCallback(0.1);

    // Divide file into 3 sections for parallel processing.
    const std::size_t section1End = fileSize / 3;
    const std::size_t section2End = (2 * fileSize) / 3;
    const std::size_t section3End = fileSize;

    std::list<Video> videos1, videos2, videos3;
    std::mutex videoMutex;
    std::vector<std::thread> threads;

    auto parseThreadFunc = [&](std::size_t startPos, std::size_t endPos, std::list<Video>& resultVideos) {
        // Each thread scans its section of the HTML file independently.
        // Results are collected into a separate list to avoid synchronization overhead.
        resultVideos = parseRange(htmlContent, startPos, endPos);
    };

    // Spawn 3 threads to process file sections simultaneously.
    threads.emplace_back(parseThreadFunc, 0, section1End, std::ref(videos1));
    threads.emplace_back(parseThreadFunc, section1End, section2End, std::ref(videos2));
    threads.emplace_back(parseThreadFunc, section2End, section3End, std::ref(videos3));

    if (progressCallback) progressCallback(0.3);

    // Wait for all threads to complete.
    for (auto& thread : threads) {
        thread.join();
    }

    if (progressCallback) progressCallback(0.7);

    // Merge results from all threads, deduplicating by video ID.
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

    std::cout << "[WatchHistoryParser] Multi-threaded parse complete: \""
              << filePath << "\" extracted " << mergedVideos.size() << " unique videos\\n";

    return mergedVideos;
}

std::list<Video> WatchHistoryParser::parseRange(const std::string& htmlContent, std::size_t startPos, std::size_t endPos) {
    // Parse a range of HTML content, starting from startPos up to (but not including) endPos.
    // To avoid missing hrefs that span section boundaries, we search backwards from startPos
    // to find the previous href boundary and forwards from endPos to find the next boundary.

    std::list<Video> videos;
    std::unordered_set<std::string> seenVideoIds;

    // Adjust boundaries to avoid splitting href attributes.
    if (startPos > 0) {
        // Back up to previous href=" boundary.
        while (startPos > 0 && htmlContent.substr(startPos, 6) != "href=\"") {
            startPos--;
        }
    }

    if (endPos < htmlContent.size()) {
        // Move forward to the next href=" boundary or end of file.
        while (endPos < htmlContent.size() && htmlContent.substr(endPos, 6) != "href=\"") {
            endPos++;
        }
    }

    // Scan for href attributes within this adjusted range.
    std::size_t searchPos = startPos;
    while (searchPos < endPos) {
        std::size_t hrefPos = htmlContent.find("href=\"", searchPos);
        if (hrefPos == std::string::npos || hrefPos >= endPos) {
            break;
        }

        std::size_t valueStart = hrefPos + 6;
        std::size_t valueEnd = htmlContent.find('"', valueStart);
        if (valueEnd == std::string::npos || valueEnd > endPos) {
            break;
        }

        std::string href = htmlContent.substr(valueStart, valueEnd - valueStart);
        auto videoId = extractVideoIdFromHref(href);

        if (videoId.has_value() && seenVideoIds.insert(*videoId).second) {
            videos.emplace_back(*videoId, "", "", "", 0, std::list<std::string>{});
        }

        searchPos = valueEnd + 1;
    }

    return videos;
}
