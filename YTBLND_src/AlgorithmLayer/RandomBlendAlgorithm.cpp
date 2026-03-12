#include "RandomBlendAlgorithm.h"

#include <algorithm>
#include <random>
#include <vector>
#include <chrono>

RandomBlendAlgorithm::RandomBlendAlgorithm(int videosPerUser)
    : videosPerUser(videosPerUser) {}

Blend RandomBlendAlgorithm::generateBlend(const std::list<User>& participants) {
    std::list<Video> combined;

    // Seed RNG from the system clock so each blend is different
    unsigned seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);

    for (const User& user : participants) {
        std::list<Video> watchLater = user.getYouTubeData().getWatchLaterVideos();

        // Copy to vector for random_shuffle / sample
        std::vector<Video> pool(watchLater.begin(), watchLater.end());
        std::shuffle(pool.begin(), pool.end(), rng);

        int count = std::min(videosPerUser, static_cast<int>(pool.size()));
        for (int i = 0; i < count; ++i) {
            combined.push_back(pool[i]);
        }
    }

    // Build a simple blendID from participant IDs
    std::string blendID;
    for (const User& user : participants) {
        if (!blendID.empty()) blendID += "_";
        blendID += user.getUserID();
    }
    blendID += "_random";

    return Blend(blendID, "random", participants, combined);
}
