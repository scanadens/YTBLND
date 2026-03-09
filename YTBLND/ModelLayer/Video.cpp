#include "Video.h"

Video::Video(const std::string& videoID,
             const std::string& title,
             const std::string& channelID,
             const std::string& thumbnailURL,
             int duration,
             const std::list<std::string>& tags)
    : videoID(videoID),
      title(title),
      channelID(channelID),
      thumbnailURL(thumbnailURL),
      duration(duration),
      tags(tags) {}

std::string Video::getVideoID() const {
    return videoID;
}

std::string Video::getTitle() const {
    return title;
}

std::string Video::getChannelID() const {
    return channelID;
}

std::string Video::getThumbnailURL() const {
    return thumbnailURL;
}

int Video::getDuration() const {
    return duration;
}

std::list<std::string> Video::getTags() const {
    return tags;
}