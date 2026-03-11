#ifndef VIDEOENTRY_H
#define VIDEOENTRY_H

#include "Video.h"

class VideoEntry {
    private:
        Video video;
        int watchCount;
        int lastWatched;
        
    public:
        VideoEntry(const Video& video, int watchCount, int lastWatched);

        // Getters
        Video getVideo()       const;
        int   getWatchCount()  const;
        int   getLastWatched() const;

        // Setters — watch data can be updated as the user's activity changes
        void setWatchCount(int watchCount);
        void setLastWatched(int lastWatched);
};

#endif