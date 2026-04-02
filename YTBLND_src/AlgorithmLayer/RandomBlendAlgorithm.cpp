#include "RandomBlendAlgorithm.hpp"

#include <algorithm>
#include <random>
#include <vector>
#include <chrono>

RandomBlendAlgorithm::RandomBlendAlgorithm(int videosPerUser)
    : videosPerUser(videosPerUser) {}

Blend RandomBlendAlgorithm::generateBlend(const std::list<User>& participants,
                                          const std::string& title) {
    std::list<Video> combined;

    // Seed RNG from the system clock so each blend is different
    unsigned seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);

    // collect a global pool of available videos (for filling empty slots)
    std::vector<Video> globalPool;

    for (const User& user : participants) {
        std::list<Video> watchLater = user.getYouTubeData().getWatchLaterVideos();

        // Copy to vector for random_shuffle / sample
        std::vector<Video> pool(watchLater.begin(), watchLater.end());
        std::shuffle(pool.begin(), pool.end(), rng);

        int poolSize = static_cast<int>(pool.size());
        // add pool contents to global pool for later filling
        if (poolSize > 0) {
            globalPool.insert(globalPool.end(), pool.begin(), pool.end());
            // Fill videosPerUser slots for this user (allow duplicates by wrapping)
            for (int i = 0; i < videosPerUser; ++i) {
                combined.push_back(pool[i % poolSize]);
            }
        }
    }

    // If some users had no videos, fill remaining slots from the global pool
    int desiredTotal = static_cast<int>(participants.size()) * videosPerUser;
    if (static_cast<int>(combined.size()) < desiredTotal && !globalPool.empty()) {
        std::uniform_int_distribution<size_t> dist(0, globalPool.size() - 1);
        while (static_cast<int>(combined.size()) < desiredTotal) {
            combined.push_back(globalPool[dist(rng)]);
        }
    }

    // Shuffle final selection so videos are mixed across users
    std::vector<Video> finalVec(combined.begin(), combined.end());
    std::shuffle(finalVec.begin(), finalVec.end(), rng);
    combined.assign(finalVec.begin(), finalVec.end());

    // Build a simple blendID from participant IDs
    std::string blendID;
    for (const User& user : participants) {
        if (!blendID.empty()) blendID += "_";
        blendID += user.getUserID();
    }
    blendID += "_random";

    return Blend(blendID, title, "random", participants, combined);
}
