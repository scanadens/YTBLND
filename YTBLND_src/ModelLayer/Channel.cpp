#include "Channel.h"

Channel::Channel(const std::string& channelID,
                 const std::string& displayName,
                 const std::list<std::string>& categories)
    : channelID(channelID), displayName(displayName), categories(categories) {}

// Getters

std::string Channel::getChannelID() const {
    return channelID;
}

std::string Channel::getDisplayName() const {
    return displayName;
}

std::list<std::string> Channel::getCategories() const {
    return categories;
}