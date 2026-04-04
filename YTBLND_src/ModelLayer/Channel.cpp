/**
 * \file Channel.cpp
 * \brief Implementation for Channel.
 * \author Jasmine Kumar
 */

#include "Channel.hpp"
#include "JsonUtils.hpp"

#include <sstream>

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

std::string Channel::toString() const {
    std::ostringstream categoriesJson;
    categoriesJson << "[";
    bool first = true;
    for (const auto& category : categories) {
        if (!first) {
            categoriesJson << ",";
        }
        categoriesJson << ModelJson::quote(category);
        first = false;
    }
    categoriesJson << "]";

    return "{"
           "\"channel_id\":" + ModelJson::quote(channelID) + ","
           "\"display_name\":" + ModelJson::quote(displayName) + ","
           "\"categories\":" + categoriesJson.str()
           + "}";
}