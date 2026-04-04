#ifndef WEIGHTED_BLEND_ALGORITHM_H
#define WEIGHTED_BLEND_ALGORITHM_H

#include "IBlendAlgorithm.hpp"
#include <vector>

/**
 * \file WeightedBlendAlgorithm.hpp
 * \brief Weighted Blend implementation of IBlendAlgorithm
 * \author Shamar Pennant
 * \author Jasmine Kumar
 * 
 * Blend algorithm that generates a blend by weighing contributions from
 * each participant. Participants with higher weights will have more of
 * their videos selected for the final blend.
 */
/**
 * \class WeightedBlendAlgorithm
 * \brief WeightedBlendAlgorithm class declaration.
 */
class WeightedBlendAlgorithm : public IBlendAlgorithm {
public:
    /**
     * Constructs a WeightedBlendAlgorithm with default weight distribution.
     * \param videosPerUser  How many videos to select from each participant.
     *                       If a user has fewer videos than this, all of their
     *                       videos are included.
     */
    explicit WeightedBlendAlgorithm(int videosPerUser = 5);

    /**
     * Constructs a WeightedBlendAlgorithm with custom participant weights.
     * \param weights        Vector of weights for each participant, in order.
     *                       Weights should be normalized or will be normalized internally.
     * \param videosPerUser  How many videos to select from each participant.
     */
    WeightedBlendAlgorithm(const std::vector<double>& weights, int videosPerUser = 5);

    /**
     * Generates a weighted Blend from a collection of participants.
     * Each participant contributes videos based on their assigned weight.
     * 
     * \param participants List of users whose Watch Later data is used as input.
     * \param title        User-facing name for the blend (default: empty).
     * \return A new Blend containing weighted video selections from participants.
     */
    Blend generateBlend(const std::list<User>& participants,
                        const std::string& title = "") override;

private:
    /**
     * Normalizes the weight vector so that weights sum to 1.0.
     * \param weights Vector of weights to normalize.
     */
    void normalizeWeights(std::vector<double>& weights);

    /**
     * Selects videos from a participant based on their weight contribution.
     * \param user   The participant to select videos from.
     * \param weight The weight factor for this participant (0.0 to 1.0).
     * \return List of selected videos.
     */
    std::list<Video> selectWeightedVideos(const User& user, double weight);

    /** Vector of weights assigned to participants. */
    std::vector<double> participantWeights;

    /** Number of videos to attempt to select from each participant. */
    int videosPerUser;
};

#endif
