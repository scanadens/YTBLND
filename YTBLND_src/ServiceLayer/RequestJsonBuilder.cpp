#include "RequestJsonBuilder.hpp"

#include "../ModelLayer/JsonUtils.hpp"

#include <sstream>

namespace RequestJsonBuilder {

std::string buildRegisterJson(const std::string& userID,
                  const std::string& username,
                  const std::string& email,
                  const std::string& password) {
    return "{"
        "\"user_id\":" + ModelJson::quote(userID) + ","
        "\"username\":" + ModelJson::quote(username) + ","
        "\"email\":" + ModelJson::quote(email) + ","
        "\"password\":" + ModelJson::quote(password)
        + "}";
}

std::string buildLoginJson(const std::string& userID, const std::string& password) {
    return "{"
           "\"user_id\":" + ModelJson::quote(userID) + ","
           "\"password\":" + ModelJson::quote(password)
           + "}";
}

std::string buildBlendJson(const std::string& blendID,
                           const std::string& creatorID,
                           const std::string& algorithm,
                           const std::vector<std::string>& participants) {
    std::ostringstream participantsJson;
    participantsJson << "[";
    for (size_t i = 0; i < participants.size(); ++i) {
        if (i > 0) {
            participantsJson << ",";
        }
        participantsJson << ModelJson::quote(participants[i]);
    }
    participantsJson << "]";

    return "{"
           "\"blend_id\":" + ModelJson::quote(blendID) + ","
           "\"creator_id\":" + ModelJson::quote(creatorID) + ","
           "\"algorithm\":" + ModelJson::quote(algorithm) + ","
           "\"participants\":" + participantsJson.str()
           + "}";
}

std::string buildWatchLaterJson(const std::list<Video>& videos) {
    std::ostringstream videosJson;
    videosJson << "[";

    bool first = true;
    for (const auto& video : videos) {
        if (!first) {
            videosJson << ",";
        }
        first = false;

        videosJson << "{"
                   << "\"video_id\":" << ModelJson::quote(video.getVideoID()) << ","
                   << "\"title\":" << ModelJson::quote(video.getTitle()) << ","
                   << "\"channel_id\":" << ModelJson::quote(video.getChannelID()) << ","
                   << "\"channel_name\":" << ModelJson::quote(video.getChannelName()) << ","
                   << "\"thumbnail_url\":" << ModelJson::quote(video.getThumbnailURL()) << ","
                   << "\"channel_logo_url\":" << ModelJson::quote(video.getChannelLogoURL())
                   << "}";
    }

    videosJson << "]";
    return "{\"videos\":" + videosJson.str() + "}";
}

} // namespace RequestJsonBuilder
