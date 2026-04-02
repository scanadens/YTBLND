#ifndef BLEND_H
#define BLEND_H

/**
 * \file Blend.hpp
 * \brief Model representing a generated video feed shared by a group of users.
 *  \author Jasmine Kumar
 *
 * A Blend is produced by an IBlendAlgorithm from the Watch Later lists of two
 * or more participants.  It is identified by a UUID-style blendID and stores
 * both the resulting video list and the participants that contributed to it.
 */

#include <string>
#include <list>

#include "User.hpp"
#include "Video.hpp"

/// A generated video feed shared by a group of blend participants.
class Blend {
    private:
        std::string blendID;
        std::string title;
        std::string algorithmUsed;
        std::list<User> participants;
        std::list<Video> videoList;

    public:
        /**
         * Constructs a Blend with all required fields.
         * \param blendID       Unique identifier for this blend.
         * \param title         User-facing name for this blend.
         * \param algorithmUsed Name of the IBlendAlgorithm that created it.
         * \param participants  Users whose Watch Later data was used as input.
         * \param videoList     Ordered list of videos selected by the algorithm.
         */
        Blend(const std::string& blendID,
              const std::string& title,
              const std::string& algorithmUsed,
              const std::list<User>& participants,
              const std::list<Video>& videoList);

        /// \return Unique blend identifier.
        std::string      getBlendID()       const;
        /// \return User-facing title of this blend.
        std::string      getTitle()         const;
        /// \return Name of the algorithm used to generate this blend.
        std::string      getAlgorithmUsed() const;
        /// \return List of users who participated in this blend.
        std::list<User>  getParticipants()  const;
        /// \return Ordered list of videos in this blend.
        std::list<Video> getVideoList()     const;

        /**
         * Replaces the current video list (e.g. after API enrichment).
         * \param videoList New video list to store.
         */
        void setVideoList(const std::list<Video>& videoList);

        /**
         * Returns the video at a given position, wrapping around the list.
         * \param index Zero-based index; behaviour is undefined for negative values.
         * \return Video at the requested position.
         */
        Video getVideo(int index) const;

        /// \return Number of videos in the blend.
        int size() const;

        /// \return JSON string representation of this blend.
        std::string toString() const;
};

#endif
