#ifndef CHATROOM_H
#define CHATROOM_H

/**
 * \file ChatRoom.hpp
 * \brief In-memory chat session tied to a specific Blend.
 *  \author Jasmine Kumar
 *
 * Identified by the blendID of the Blend it belongs to.  Messages are not
 * persisted in this version — all chat history is lost when the session ends.
 *
 * Currently only one ChatRoom is active at a time (the one matching the active
 * Blend in AppState), but AppState holds a map<string, ChatRoom> keyed by
 * blendID, so supporting multiple simultaneous rooms requires no changes here.
 */

#include <string>
#include <list>
#include <vector>

#include "Message.hpp"

/// In-memory chat session associated with a Blend.
class ChatRoom {
    private:
        std::string              blendID;         ///< ID of the owning Blend.
        std::vector<std::string> participantIDs;  ///< userIDs pulled from the Blend on construction.
        std::list<Message>       messages;        ///< In-memory history, oldest first.

    public:
        /**
         * Constructs a ChatRoom for a given blend.
         * \param blendID        ID of the Blend this room belongs to.
         * \param participantIDs userIDs of all blend participants.
         */
        ChatRoom(const std::string& blendID,
                 const std::vector<std::string>& participantIDs);

        /// \return ID of the Blend this room belongs to.
        std::string              getBlendID()        const;
        /// \return List of participant userIDs.
        std::vector<std::string> getParticipantIDs() const;
        /// \return In-memory message history, oldest first.
        std::list<Message>       getMessages()        const;

        /**
         * Appends a new message to the room's history.
         * \param userID ID of the sending user.
         * \param text   Message content.
         */
        void addMessage(const std::string& userID, const std::string& text);

        /**
         * Returns true if the given userID is a participant in this room.
         * Used to validate that only blend participants can send messages.
         * \param userID ID to check.
         * \return true if the user is a participant.
         */
        bool isParticipant(const std::string& userID) const;

        /**
         * Clears all messages from memory.
         * Has no effect on persistence since persistence is not implemented yet.
         */
        void clearMessages();

        std::string toString() const;
};

#endif
