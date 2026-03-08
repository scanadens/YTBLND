#ifndef VIDEOENTRY_H
#define VIDEOENTRY_H

#include "Video.h"

class VideoEntry : Video {
    private:
        Video video;
        int watchCount;
        int lastWatched;
};
#endif