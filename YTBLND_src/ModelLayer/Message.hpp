#ifndef MESSAGE_H
#define MESSAGE_H

/**
 * \file Message.hpp
 * \brief A single chat message sent within a ChatRoom.
 * \author Jasmine Kumar
 *
 * Kept as a plain struct since it is pure data with no behaviour.
 */

#include <string>
#include <ctime>

/**
 * \brief A single chat message sent within a ChatRoom.
 */
/**
 * \struct Message
 * \brief Message class declaration.
 */
struct Message {
    std::string userID;    ///< ID of the User who sent this message.
    std::string text;      ///< Content of the message.
    std::time_t timestamp; ///< Unix timestamp of when the message was sent.

    /**
     * Constructs a Message with an optional timestamp.
     * \param userID    ID of the sending user.
     * \param text      Message content.
     * \param timestamp Unix timestamp; defaults to the current time.
     */
    Message(const std::string& userID,
            const std::string& text,
            std::time_t timestamp = std::time(nullptr))
        : userID(userID), text(text), timestamp(timestamp) {}

    /**
     * String representation of \c Message
     * \return \c string 
     */
    std::string toString() const;
};

#endif
