#ifndef YOUTUBEDATA_H
#define YOUTUBEDATA_H

/**
 * \file YouTubeData.hpp
 * \brief Container for all YouTube data associated with a User.
 *  \author Jasmine Kumar
 *
 * Holds the user's watch history, liked videos, subscribed channels, and
 * Watch Later playlist.  Populated in bulk from parsed Google Takeout exports.
 */

#include <list>

#include "VideoEntry.hpp"
#include "Channel.hpp"
#include "Video.hpp"

/// Container for all YouTube data associated with a User.
class YouTubeData {
    private:
        std::list<VideoEntry> watchHistory;
        std::list<Video>      likedVideos;
        std::list<Channel>    subscribedChannels;
        std::list<Video>      watchLaterVideos;

    public:
        YouTubeData();

        /// \return Full watch-history list (VideoEntry objects with watch counts).
        std::list<VideoEntry> getWatchHistory()      const;
        /// \return List of videos the user has liked.
        std::list<Video>      getLikedVideos()        const;
        /// \return List of channels the user is subscribed to.
        std::list<Channel>    getSubscribedChannels() const;
        /// \return Watch Later playlist videos.
        std::list<Video>      getWatchLaterVideos()   const;

        /// \param watchHistory Replaces the full watch-history list.
        void setWatchHistory(const std::list<VideoEntry>& watchHistory);
        /// \param likedVideos Replaces the liked-videos list.
        void setLikedVideos(const std::list<Video>& likedVideos);
        /// \param subscribedChannels Replaces the subscribed-channels list.
        void setSubscribedChannels(const std::list<Channel>& subscribedChannels);
        /// \param videos Replaces the Watch Later playlist.
        void setWatchLaterVideos(const std::list<Video>& videos);

        /**
         * Returns the n most-replayed videos from the watch history.
         * \param n Maximum number of results to return.
         * \return List of up to n Videos sorted by descending watch count.
         */
        std::list<Video> getMostWatchedVideos(int n) const;

        /**
         * Returns the n most frequently watched channels from the watch history.
         * \param n Maximum number of results to return.
         * \return List of up to n Channels sorted by descending view count.
         */
        std::list<Channel> getTopChannels(int n) const;

        /// \return JSON string representation of this YouTubeData object.
        std::string toString() const;
};

#endif
