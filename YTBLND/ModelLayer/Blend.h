#ifndef BLEND_H
#define BLEND_H

#include<string>
#include<list>\

#include "Video.h"

class BLEND {
    private:
        std::string blendID, algorithmUsed;
        std::list<Video> videoList;
    public:
        Video getVideo(int index); //returns video at a given position in videoList
        int size(); //returns total number of videos in the blend
};
#endif