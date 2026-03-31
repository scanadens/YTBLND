#ifndef FRIEND_H
#define FRIEND_H

/**
 * \file Friend.hpp
 * \brief Model representing a friend/contact entry on a User's list.
 *  \author Jasmine Kumar
 */

#include <string>

/// A friend or contact entry stored on a User's friends list.
class Friend {
    private:
        std::string userID;
        std::string displayName;
        std::string email;

    public:
        /**
         * Constructs a Friend entry.
         * \param userID      Unique identifier of the friend's account.
         * \param displayName Display name shown in the UI.
         * \param email       Email address of the friend.
         */
        Friend(const std::string& userID,
               const std::string& displayName,
               const std::string& email);

        /// \return Unique identifier of the friend's account.
        std::string getUserID()      const;
        /// \return Display name of the friend.
        std::string getDisplayName() const;
        /// \return Email address of the friend.
        std::string getEmail()       const;

        /// \param displayName New display name.
        void setDisplayName(const std::string& displayName);
        /// \param email New email address.
        void setEmail(const std::string& email);

        std::string toString() const;
};

#endif
