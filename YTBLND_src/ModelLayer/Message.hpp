#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <ctime>

// A single chat message sent within a ChatRoom.
// Kept as a struct since it is pure data with no behaviour.
struct Message {
    std::string userID;    // ID of the User who sent this message
    std::string text;      // Content of the message
    std::time_t timestamp; // Unix timestamp of when the message was sent

    Message(const std::string& userID,
            const std::string& text,
            std::time_t timestamp = std::time(nullptr))
        : userID(userID), text(text), timestamp(timestamp) {}
};

#endif