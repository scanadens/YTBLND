#include "YouTubeData.h"
#include <map>
#include <vector>
#include <algorithm>

YouTubeData::YouTubeData() {}

// Getters 
std::list<VideoEntry> YouTubeData::getWatchHistory() const {
    return watchHistory;
}

std::list<Video> YouTubeData::getLikedVideos() const {
    return likedVideos;
}

std::list<Channel> YouTubeData::getSubscribedChannels() const {
    return subscribedChannels;
}

//Setters 

void YouTubeData::setWatchHistory(const std::list<VideoEntry>& watchHistory) {
    this->watchHistory = watchHistory;
}

void YouTubeData::setLikedVideos(const std::list<Video>& likedVideos) {
    this->likedVideos = likedVideos;
}

void YouTubeData::setSubscribedChannels(const std::list<Channel>& subscribedChannels) {
    this->subscribedChannels = subscribedChannels;
}

std::list<Video> YouTubeData::getWatchLaterVideos() const {
    return watchLaterVideos;
}

void YouTubeData::setWatchLaterVideos(const std::list<Video>& videos) {
    this->watchLaterVideos = videos;
}

// Computed Queries

std::list<Video> YouTubeData::getMostWatchedVideos(int n) const {
    // Copy watch history into a vector for sorting
    std::vector<VideoEntry> entries(watchHistory.begin(), watchHistory.end());

    // Sort copied vector
    std::sort(entries.begin(), entries.end(), 
        [](const VideoEntry& a, const VideoEntry& b) {
            return a.getWatchCount() > b.getWatchCount();
        });
    // Add sorted results to a list
    std::list<Video> result;
    int count = 0;
    for (const auto& entry : entries) {
        if (count >= n) break;
        result.push_back(entry.getVideo());
        count++;
    }
    return result;
}

std::list<Channel> YouTubeData::getTopChannels(int n) const {
    // Count how many watched videos belong to each channelID
    std::map<std::string, int> channelWatchCounts;
    for (const auto& entry : watchHistory) {
        const std::string& cid = entry.getVideo().getChannelID();
        channelWatchCounts[cid] += entry.getWatchCount();
    }

    // Sort subscribedChannels by their accumulated watch count
    std::vector<Channel> channels(subscribedChannels.begin(), subscribedChannels.end());
    std::sort(channels.begin(), channels.end(),
        [&channelWatchCounts](const Channel& a, const Channel& b) {
            int countA = channelWatchCounts.count(a.getChannelID()) ? channelWatchCounts.at(a.getChannelID()) : 0;
            int countB = channelWatchCounts.count(b.getChannelID()) ? channelWatchCounts.at(b.getChannelID()) : 0;
            return countA > countB;
        });

    std::list<Channel> result;
    int count = 0;
    for (const auto& channel : channels) {
        if (count >= n) break;
        result.push_back(channel);
        count++;
    }
    return result;
}