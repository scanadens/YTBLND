#ifndef CHATROOM_H
#define CHATROOM_H

#include <string>
#include <list>
#include <vector>

#include "Message.h"

// ChatRoom represents an in-memory chat session tied to a specific Blend.
// It is identified by the blendID of the Blend it belongs to.
//
// Currently only one ChatRoom is active at a time (the one matching the
// active Blend in AppState), but the design supports multiple simultaneous
// rooms — AppState holds a map<string, ChatRoom> keyed by blendID, so
// expanding to multiple active rooms requires no changes to this class.
//
// Messages are not persisted in this version. All chat history is lost
// when the session ends.
class ChatRoom {
    private:
        std::string          blendID;      // ID of the Blend this room belongs to
        std::vector<std::string> participantIDs; // userIDs pulled from the Blend on construction
        std::list<Message>   messages;     // In-memory message history, oldest first

    public:
        ChatRoom(const std::string& blendID,
                 const std::vector<std::string>& participantIDs);

        // Getters
        std::string              getBlendID()        const;
        std::vector<std::string> getParticipantIDs() const;
        std::list<Message>       getMessages()        const;

        // Messaging
        void addMessage(const std::string& userID, const std::string& text);

        // Returns true if the given userID is a participant in this room.
        // Used to validate that only blend participants can send messages.
        bool isParticipant(const std::string& userID) const;

        // Clears all messages from memory. Does not affect persistence
        // since persistence is not implemented yet.
        void clearMessages();
};

#endif