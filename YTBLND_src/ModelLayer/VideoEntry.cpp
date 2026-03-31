#include "VideoEntry.hpp"

VideoEntry::VideoEntry(const Video& video, int watchCount, int lastWatched)
    : video(video), watchCount(watchCount), lastWatched(lastWatched) {}

// Getters 

Video VideoEntry::getVideo() const {
    return video;
}

int VideoEntry::getWatchCount() const {
    return watchCount;
}

int VideoEntry::getLastWatched() const {
    return lastWatched;
}

// Setters 
void VideoEntry::setWatchCount(int watchCount) {
    this->watchCount = watchCount;
}

void VideoEntry::setLastWatched(int lastWatched) {
    this->lastWatched = lastWatched;
}

std::string VideoEntry::toString() const {
    return "{"
           "\"video\":" + video.toString() + ","
           "\"watch_count\":" + std::to_string(watchCount) + ","
           "\"last_watched\":" + std::to_string(lastWatched)
           + "}";
}
