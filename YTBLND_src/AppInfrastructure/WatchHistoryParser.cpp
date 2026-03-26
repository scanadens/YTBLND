#include "WatchHistoryParser.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

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
