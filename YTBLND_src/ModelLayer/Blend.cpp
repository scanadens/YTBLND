#include "Blend.h"
#include <stdexcept>

Blend::Blend(const std::string& blendID,
             const std::string& algorithmUsed,
             const std::list<User>& participants,
             const std::list<Video>& videoList)
    : blendID(blendID),
      algorithmUsed(algorithmUsed),
      participants(participants),
      videoList(videoList) {}

// Getters 

std::string Blend::getBlendID() const {
    return blendID;
}

std::string Blend::getAlgorithmUsed() const {
    return algorithmUsed;
}

std::list<User> Blend::getParticipants() const {
    return participants;
}

std::list<Video> Blend::getVideoList() const {
    return videoList;
}

// Setters 

void Blend::setVideoList(const std::list<Video>& videoList) {
    this->videoList = videoList;
}

// Indexed Access 

Video Blend::getVideo(int index) const {
    if (index < 0 || index >= static_cast<int>(videoList.size())) {
        throw std::out_of_range("Blend::getVideo — index " + std::to_string(index) +
                                " is out of range (size = " + std::to_string(videoList.size()) + ")");
    }
    auto it = videoList.begin();
    std::advance(it, index);
    return *it;
}

int Blend::size() const {
    return static_cast<int>(videoList.size());
}