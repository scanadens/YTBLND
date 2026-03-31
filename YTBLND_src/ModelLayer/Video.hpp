#ifndef VIDEO_H
#define VIDEO_H

/**
 * \file Video.hpp
 * \brief Immutable data model for a single YouTube video.
 *  \author Jasmine Kumar
 *
 * Populated initially with only a videoID from the Watch Later CSV; the
 * remaining fields are filled in by YouTubeMetadataFetcher after an API call.
 */

#include <string>
#include <list>

/// Immutable data model for a single YouTube video.
class Video {
    private:
        std::string videoID;
        std::string title;
        std::string channelID;
        std::string thumbnailURL;
        int         duration;
        std::list<std::string> tags;
        std::string channelName;
        std::string channelLogoURL;

    public:
        /**
         * Constructs a Video with all metadata fields.
         * \param videoID       YouTube video ID (e.g. "dQw4w9WgXcQ").
         * \param title         Video title returned by the Data API.
         * \param channelID     YouTube channel ID of the uploader.
         * \param thumbnailURL  URL of the best-available thumbnail image.
         * \param duration      Video duration in seconds (0 if unknown).
         * \param tags          List of tags associated with the video.
         * \param channelName   Display name of the uploading channel.
         * \param channelLogoURL URL of the channel's profile picture.
         */
        Video(const std::string& videoID,
            const std::string& title,
            const std::string& channelID,
            const std::string& thumbnailURL,
            int duration,
            const std::list<std::string>& tags,
            const std::string& channelName    = "",
            const std::string& channelLogoURL = "");

        /// \return YouTube video ID string.
        std::string getVideoID()        const;
        /// \return Video title (empty if not yet enriched by the API).
        std::string getTitle()          const;
        /// \return YouTube channel ID of the uploader.
        std::string getChannelID()      const;
        /// \return URL of the video thumbnail (empty if not yet enriched).
        std::string getThumbnailURL()   const;
        /// \return Duration in seconds (0 if unknown).
        int         getDuration()       const;
        /// \return List of tags associated with the video.
        std::list<std::string> getTags() const;
        /// \return Display name of the uploading channel.
        std::string getChannelName()    const;
        /// \return URL of the channel's profile picture.
        std::string getChannelLogoURL() const;

        /// \return JSON string representation of this video.
        std::string toString() const;
};

#endif
