#include "Message.hpp"
#include "JsonUtils.hpp"

std::string Message::toString() const {
    return "{"
           "\"sender_id\":" + ModelJson::quote(userID) + ","
           "\"content\":" + ModelJson::quote(text) + ","
           "\"sent_at\":" + ModelJson::quote(ModelJson::toIso8601(timestamp))
           + "}";
}
