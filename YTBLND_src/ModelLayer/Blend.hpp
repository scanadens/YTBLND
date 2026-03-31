#ifndef BLEND_H
#define BLEND_H

#include<string>
#include<list>

#include "User.hpp"
#include "Video.hpp"

class Blend {
    private:
        std::string blendID;
        std::string algorithmUsed;
        std::list<User> participants;
        std::list<Video> videoList;
    public:
        Blend(const std::string& blendID,
              const std::string& algorithmUsed,
              const std::list<User>& participants,
              const std::list<Video>& videoList);

        // Getters
        std::string      getBlendID()       const;
        std::string      getAlgorithmUsed() const;
        std::list<User>  getParticipants()  const;
        std::list<Video> getVideoList()     const;

        // Setters 
        void setVideoList(const std::list<Video>& videoList);

        // Indexed access for videos
        Video getVideo(int index) const;

        // Size of the blend
        int size() const;

        std::string toString() const;
    };
#endif