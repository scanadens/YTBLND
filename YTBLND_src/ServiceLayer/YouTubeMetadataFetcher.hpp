// ============================================================================
// YouTubeMetadataFetcher.hpp
//
// Enriches a list of Video objects (video-ID only) with full snippet metadata
// from the YouTube Data API v3:
//
//   videos.list?part=snippet&id=<ids>&key=<apiKey>
//     -> title, channelId, channelTitle, thumbnailUrl
//
//   channels.list?part=snippet&id=<channelIds>&key=<apiKey>
//     -> channel logo URL (snippet.thumbnails.default.url)
//
// IDs are batched at most 50 per request (YouTube API limit).
// Videos not returned by the API (private, deleted) are kept unchanged.
// Any network or parse error is logged to stderr; the remaining videos are
// still returned with whatever data was successfully retrieved.
// ============================================================================

#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <vector>

#include <curl/curl.h>

#include "../ModelLayer/Video.hpp"

/**
 * \file YouTubeMetadataFetcher.hpp
 * \author Jasmine Kumar
 * \brief Enriches a list of Video objects (video-ID only) with full snippet metadata from the YouTube Data API v3
 * 
 * videos.list?part=snippet&id=<ids>&key=<apiKey>
 *      -> title, channelId, channelTitle, thumbnailUrl
 * channels.list?part=snippet&id=<channelIds>&key=<apiKey>
 *      -> channel logo URL (snippet.thumbnails.default.url)
 * 
 * IDs are batched at most 50 per request (YouTube API limit).
 * Videos not returned by the API (private, deleted) are kept unchanged.
 * Any network or parse error is logged to stderr; the remaining videos are
 * still returned with whatever data was successfully retrieved.
 */

/**
 * \class YouTubeMetadataFetcher
 * \brief YouTubeMetadataFetcher class declaration.
 */
class YouTubeMetadataFetcher {
public:
    explicit YouTubeMetadataFetcher(std::string apiKey);

    /**
     * Enriches a list of Videos that only have videoID populated.
     * Returns a new list where each Video has title, channelID, channelName,
     * thumbnailURL, and channelLogoURL filled in from the API.
     */
    std::list<Video> enrich(const std::list<Video>& videos);

private:
    std::string apiKey;

    static constexpr int kBatchSize = 50;

/**
 * \struct VideoMeta
 * \brief VideoMeta class declaration.
 */
    struct VideoMeta {
        std::string title;
        std::string channelId;
        std::string channelName;
        std::string thumbnailUrl;
    };

    // Fetches snippet data for up to kBatchSize video IDs.
    // Returns a map of videoId -> VideoMeta.
    std::unordered_map<std::string, VideoMeta>
    fetchVideoBatch(const std::vector<std::string>& ids);

    // Fetches the default thumbnail URL for each channel ID.
    // Returns a map of channelId -> logoUrl.
    std::unordered_map<std::string, std::string>
    fetchChannelLogos(const std::vector<std::string>& channelIds);

    // Performs a GET request and returns the response body.
    // Throws std::runtime_error on curl failure.
    std::string httpGet(const std::string& url);

    static size_t writeCallback(void* contents, size_t size, size_t nmemb,
                                std::string* out);
};
