#ifndef VIDEOENTRY_H
#define VIDEOENTRY_H

/**
 * \file VideoEntry.hpp
 * \brief A Video decorated with watch-history metadata.
 *  \author Jasmine Kumar
 *
 * Wraps a Video with a watch count and a last-watched timestamp so the
 * watch-history list can record how many times a user has seen a video.
 */

#include "Video.hpp"

/// A Video decorated with watch-history metadata.
class VideoEntry {
    private:
        Video video;
        int watchCount;
        int lastWatched;

    public:
        /**
         * Constructs a VideoEntry.
         * \param video       The underlying Video object.
         * \param watchCount  Number of times the user has watched this video.
         * \param lastWatched Unix timestamp of the most recent watch event.
         */
        VideoEntry(const Video& video, int watchCount, int lastWatched);

        /// \return The underlying Video.
        Video getVideo()       const;
        /// \return Number of times the user has watched this video.
        int   getWatchCount()  const;
        /// \return Unix timestamp of the most recent watch event.
        int   getLastWatched() const;

        /// \param watchCount Updated watch count.
        void setWatchCount(int watchCount);
        /// \param lastWatched Updated last-watched Unix timestamp.
        void setLastWatched(int lastWatched);

        std::string toString() const;
};

#endif
