#ifndef YOUTUBEDATA_H
#define YOUTUBEDATA_H

#include <list>

#include "VideoEntry.hpp"
#include "Channel.hpp"
#include "Video.hpp"

class YouTubeData {
    private:
        std::list<VideoEntry> watchHistory;
        std::list<Video> likedVideos;
        std::list<Channel> subscribedChannels;
        std::list<Video> watchLaterVideos;

    public:
        YouTubeData();

        // Getters
        std::list<VideoEntry> getWatchHistory()        const;
        std::list<Video>      getLikedVideos()          const;
        std::list<Channel>    getSubscribedChannels()   const;
        std::list<Video>      getWatchLaterVideos()     const;

        // Setters — populated in bulk by YouTubeDataParser after an API fetch
        void setWatchHistory(const std::list<VideoEntry>& watchHistory);
        void setLikedVideos(const std::list<Video>& likedVideos);
        void setSubscribedChannels(const std::list<Channel>& subscribedChannels);
        void setWatchLaterVideos(const std::list<Video>& videos);

        //Computed queries
        std::list<Video> getMostWatchedVideos(int n)   const; //returns n most replayed videos
        std::list<Channel> getTopChannels(int n)       const; //returns n most frequently watched channels
};
#endif