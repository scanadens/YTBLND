#ifndef VIDEO_H
#define VIDEO_H

#include <string>
#include <list>

class Video {
    private:
        std::string videoID;
        std::string title;
        std::string channelID;
        std::string thumbnailURL;
        int         duration;
        std::list<std::string> tags;

    public:
        Video(const std::string& videoID,
            const std::string& title,
            const std::string& channelID,
            const std::string& thumbnailURL,
            int duration,
            const std::list<std::string>& tags);
            
        //Getters
        std::string getVideoID()      const;
        std::string getTitle()        const;
        std::string getChannelID()    const;
        std::string getThumbnailURL() const;
        int         getDuration()     const;
        std::list<std::string> getTags() const;

        std::string toString() const;
};
#endif