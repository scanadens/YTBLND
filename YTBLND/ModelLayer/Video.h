#ifndef VIDEO_H
#define VIDEO_H

#include <string>
#include <list>

class Video {
    protected:
        std::string videoID, title, channelID, thumbnailURL;
        int duration;
        std::list<std::string> tags;
};
#endif