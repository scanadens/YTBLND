/**
 * \file ChatRoom.cpp
 * \brief Implementation for ChatRoom.
 * \author Jasmine Kumar
 */

#include "ChatRoom.hpp"
#include "JsonUtils.hpp"

#include <algorithm>
#include <sstream>

ChatRoom::ChatRoom(const std::string& blendID,
                   const std::vector<std::string>& participantIDs)
    : blendID(blendID), participantIDs(participantIDs) {}

// -- Getters -------------------------------------------------------------------

std::string ChatRoom::getBlendID() const {
    return blendID;
}

std::vector<std::string> ChatRoom::getParticipantIDs() const {
    return participantIDs;
}

std::list<Message> ChatRoom::getMessages() const {
    return messages;
}

// -- Messaging -----------------------------------------------------------------

void ChatRoom::addMessage(const std::string& userID, const std::string& text) {
    messages.emplace_back(userID, text);
}

void ChatRoom::addMessage(const Message& msg) {
    messages.push_back(msg);
}

bool ChatRoom::isParticipant(const std::string& userID) const {
    return std::find(participantIDs.begin(), participantIDs.end(), userID)
           != participantIDs.end();
}

void ChatRoom::clearMessages() {
    messages.clear();
}

std::string ChatRoom::toString() const {
    std::ostringstream membersJson;
    membersJson << "[";
    bool firstMember = true;
    for (const auto& participantID : participantIDs) {
        if (!firstMember) {
            membersJson << ",";
        }
        membersJson << ModelJson::quote(participantID);
        firstMember = false;
    }
    membersJson << "]";

    std::ostringstream messagesJson;
    messagesJson << "[";
    bool firstMessage = true;
    for (const auto& message : messages) {
        if (!firstMessage) {
            messagesJson << ",";
        }
        messagesJson << "{"
                     << "\"type\":\"chat_message\"," 
                     << "\"room_id\":" << ModelJson::quote(blendID) << ","
                     << "\"sender_id\":" << ModelJson::quote(message.userID) << ","
                     << "\"content\":" << ModelJson::quote(message.text) << ","
                     << "\"sent_at\":" << ModelJson::quote(ModelJson::toIso8601(message.timestamp))
                     << "}";
        firstMessage = false;
    }
    messagesJson << "]";

    return "{"
           "\"blend_id\":" + ModelJson::quote(blendID) + ","
           "\"chat_room_id\":" + ModelJson::quote(blendID) + ","
           "\"member_users\":" + membersJson.str() + ","
           "\"messages\":" + messagesJson.str()
           + "}";
}