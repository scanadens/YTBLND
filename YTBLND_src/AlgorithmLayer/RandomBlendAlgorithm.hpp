#ifndef RANDOM_BLEND_ALGORITHM_H
#define RANDOM_BLEND_ALGORITHM_H

#include "IBlendAlgorithm.hpp"

/**
 * \brief Blend algorithm that picks a random subset of videos from each
 *        participant's Watch Later list and combines them into one Blend.
 */
class RandomBlendAlgorithm : public IBlendAlgorithm {
public:
    /**
     * \param videosPerUser  How many videos to sample from each participant.
     *                       If a user has fewer videos than this, all of their
     *                       videos are included.
     */
    explicit RandomBlendAlgorithm(int videosPerUser = 5);

    Blend generateBlend(const std::list<User>& participants) override;

private:
    int videosPerUser;
};

#endif
