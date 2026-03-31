#ifndef CHANNEL_H
#define CHANNEL_H

/**
 * \file Channel.hpp
 * \brief Model representing a YouTube channel.
 *  \author Jasmine Kumar
 */

#include <string>
#include <list>

/// A YouTube channel with display metadata.
class Channel {
    private:
        std::string channelID;
        std::string displayName;
        std::list<std::string> categories;

    public:
        /**
         * Constructs a Channel.
         * \param channelID   Unique YouTube channel identifier.
         * \param displayName Human-readable channel name.
         * \param categories  List of content categories for this channel.
         */
        Channel(const std::string& channelID,
                const std::string& displayName,
                const std::list<std::string>& categories);

        /// \return Unique YouTube channel identifier.
        std::string getChannelID()    const;
        /// \return Human-readable channel name.
        std::string getDisplayName()  const;
        /// \return Content categories for this channel.
        std::list<std::string> getCategories() const;

        std::string toString() const;
};

#endif
