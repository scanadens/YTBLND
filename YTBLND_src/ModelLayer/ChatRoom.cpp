#include "ChatRoom.hpp"
#include <algorithm>

ChatRoom::ChatRoom(const std::string& blendID,
                   const std::vector<std::string>& participantIDs)
    : blendID(blendID), participantIDs(participantIDs) {}

// ── Getters ───────────────────────────────────────────────────────────────────

std::string ChatRoom::getBlendID() const {
    return blendID;
}

std::vector<std::string> ChatRoom::getParticipantIDs() const {
    return participantIDs;
}

std::list<Message> ChatRoom::getMessages() const {
    return messages;
}

// ── Messaging ─────────────────────────────────────────────────────────────────

void ChatRoom::addMessage(const std::string& userID, const std::string& text) {
    messages.emplace_back(userID, text);
}

bool ChatRoom::isParticipant(const std::string& userID) const {
    return std::find(participantIDs.begin(), participantIDs.end(), userID)
           != participantIDs.end();
}

void ChatRoom::clearMessages() {
    messages.clear();
}