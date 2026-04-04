/**
 * \file YouTubeMetadataFetcher.cpp
 * \brief Implementation for YouTubeMetadataFetcher.
 * \author Jasmine Kumar
 */

// ============================================================================
// YouTubeMetadataFetcher.cpp
// ============================================================================

#include "YouTubeMetadataFetcher.hpp"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -- Constructor ---------------------------------------------------------------

YouTubeMetadataFetcher::YouTubeMetadataFetcher(std::string apiKey)
    : apiKey(std::move(apiKey)) {}

// -- Public: enrich ------------------------------------------------------------

std::list<Video> YouTubeMetadataFetcher::enrich(const std::list<Video>& videos) {
    if (apiKey.empty()) {
        std::cerr << "[YouTubeMetadataFetcher] No API key - returning videos unchanged\n";
        return videos;
    }

    // Collect all video IDs in order
    std::vector<std::string> allIds;
    allIds.reserve(videos.size());
    for (const auto& v : videos) allIds.push_back(v.getVideoID());

    // Fetch video metadata in batches of kBatchSize
    std::unordered_map<std::string, VideoMeta> metaMap;
    for (int i = 0; i < static_cast<int>(allIds.size()); i += kBatchSize) {
        int end = std::min(i + kBatchSize, static_cast<int>(allIds.size()));
        std::vector<std::string> batch(allIds.begin() + i, allIds.begin() + end);
        try {
            auto batchResult = fetchVideoBatch(batch);
            metaMap.insert(batchResult.begin(), batchResult.end());
        } catch (const std::exception& ex) {
            std::cerr << "[YouTubeMetadataFetcher] fetchVideoBatch failed: "
                      << ex.what() << "\n";
        }
    }

    // Collect unique channel IDs from the fetched metadata
    std::vector<std::string> channelIdList;
    {
        std::unordered_map<std::string, bool> seen;
        for (const auto& [vid, meta] : metaMap) {
            if (!meta.channelId.empty() && seen.find(meta.channelId) == seen.end()) {
                channelIdList.push_back(meta.channelId);
                seen[meta.channelId] = true;
            }
        }
    }

    // Fetch channel logos in batches of kBatchSize
    std::unordered_map<std::string, std::string> logoMap;
    for (int i = 0; i < static_cast<int>(channelIdList.size()); i += kBatchSize) {
        int end = std::min(i + kBatchSize, static_cast<int>(channelIdList.size()));
        std::vector<std::string> batch(channelIdList.begin() + i,
                                       channelIdList.begin() + end);
        try {
            auto batchLogos = fetchChannelLogos(batch);
            logoMap.insert(batchLogos.begin(), batchLogos.end());
        } catch (const std::exception& ex) {
            std::cerr << "[YouTubeMetadataFetcher] fetchChannelLogos failed: "
                      << ex.what() << "\n";
        }
    }

    // Rebuild the video list with enriched data
    std::list<Video> enriched;
    for (const auto& v : videos) {
        auto it = metaMap.find(v.getVideoID());
        if (it == metaMap.end()) {
            // API didn't return this video (private/deleted) - keep as-is
            enriched.push_back(v);
            continue;
        }
        const VideoMeta& m = it->second;

        std::string logoUrl;
        auto logoIt = logoMap.find(m.channelId);
        if (logoIt != logoMap.end()) logoUrl = logoIt->second;

        enriched.emplace_back(
            v.getVideoID(),
            m.title,
            m.channelId,
            m.thumbnailUrl,
            v.getDuration(),
            v.getTags(),
            m.channelName,
            logoUrl
        );
    }
    return enriched;
}

// -- Private: fetchVideoBatch --------------------------------------------------

std::unordered_map<std::string, YouTubeMetadataFetcher::VideoMeta>
YouTubeMetadataFetcher::fetchVideoBatch(const std::vector<std::string>& ids) {
    // Build comma-separated ID list
    std::ostringstream idParam;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i) idParam << ',';
        idParam << ids[i];
    }

    std::string url =
        "https://www.googleapis.com/youtube/v3/videos"
        "?part=snippet"
        "&id=" + idParam.str() +
        "&key=" + apiKey;

    std::string body = httpGet(url);

    std::unordered_map<std::string, VideoMeta> result;
    try {
        auto root = json::parse(body);
        for (const auto& item : root.value("items", json::array())) {
            std::string videoId = item.value("id", "");
            if (videoId.empty()) continue;

            const auto& snippet = item.value("snippet", json::object());

            VideoMeta m;
            m.title       = snippet.value("title", "");
            m.channelId   = snippet.value("channelId", "");
            m.channelName = snippet.value("channelTitle", "");

            // Prefer highest-resolution thumbnail available
            const auto& thumbs = snippet.value("thumbnails", json::object());
            for (const char* key : {"maxres", "standard", "high", "medium", "default"}) {
                if (thumbs.contains(key)) {
                    m.thumbnailUrl = thumbs[key].value("url", "");
                    if (!m.thumbnailUrl.empty()) break;
                }
            }

            result[videoId] = std::move(m);
        }
    } catch (const json::parse_error& ex) {
        throw std::runtime_error(
            std::string("YouTubeMetadataFetcher: videos JSON parse error: ") + ex.what());
    }
    return result;
}

// -- Private: fetchChannelLogos ------------------------------------------------

std::unordered_map<std::string, std::string>
YouTubeMetadataFetcher::fetchChannelLogos(const std::vector<std::string>& channelIds) {
    std::ostringstream idParam;
    for (size_t i = 0; i < channelIds.size(); ++i) {
        if (i) idParam << ',';
        idParam << channelIds[i];
    }

    std::string url =
        "https://www.googleapis.com/youtube/v3/channels"
        "?part=snippet"
        "&id=" + idParam.str() +
        "&key=" + apiKey;

    std::string body = httpGet(url);

    std::unordered_map<std::string, std::string> result;
    try {
        auto root = json::parse(body);
        for (const auto& item : root.value("items", json::array())) {
            std::string channelId = item.value("id", "");
            if (channelId.empty()) continue;

            const auto& snippet = item.value("snippet", json::object());
            const auto& thumbs  = snippet.value("thumbnails", json::object());

            // Channel logos: prefer medium (240x240) over default (88x88)
            std::string logoUrl;
            for (const char* key : {"medium", "default"}) {
                if (thumbs.contains(key)) {
                    logoUrl = thumbs[key].value("url", "");
                    if (!logoUrl.empty()) break;
                }
            }
            if (!logoUrl.empty()) result[channelId] = std::move(logoUrl);
        }
    } catch (const json::parse_error& ex) {
        throw std::runtime_error(
            std::string("YouTubeMetadataFetcher: channels JSON parse error: ") + ex.what());
    }
    return result;
}

// -- Private: httpGet ----------------------------------------------------------

std::string YouTubeMetadataFetcher::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("YouTubeMetadataFetcher: curl_easy_init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPGET,        1L);
    // Aggressive timeouts to prevent hanging on slow/unavailable API endpoints.
    // 3s connect timeout ensures socket is reaching the server quickly.
    // 10s total timeout prevents long waits for slow API responses.
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        10L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(
            std::string("YouTubeMetadataFetcher: HTTP GET failed: ") +
            curl_easy_strerror(res));
    }
    return response;
}

size_t YouTubeMetadataFetcher::writeCallback(void* contents, size_t size,
                                              size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}
