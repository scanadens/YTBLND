#ifndef YOUTUBEDATA_H
#define YOUTUBEDATA_H

#include <list>

#include "VideoEntry.h"
#include "Channel.h"
#include "Video.h"

class YouTubeData {
    private:
        std::list<VideoEntry> watchHistory;
        std::list<Video> likedVideos;
        std::list<Channel> subscribedChannels;
    public:
        std::list<Video> getMostWatchedVideos(int n); //returns n most replayed videos
        std::list<Channel> getTopChannels(int n); //returns n most frequently watched channels
};
#endif