#ifndef USER_H
#define USER_H

/**
 * \file User.hpp
 * \brief Model representing a registered application user.
 * \author Jasmine Kumar
 *
 * Holds identity fields and the user's associated YouTube data.
 * Passwords are stored as plaintext at this stage of development.
 */

#include <string>
#include <list>

#include "Friend.hpp"
#include "YouTubeData.hpp"

/** A registered application user. */
/**
 * \class User
 * \brief User class declaration.
 */
class User {
    private:
        std::string userID; // generated id
        std::string username; // provided username
        std::string email; // provided user email
        std::string password;   ///< Stored as plaintext - hashing is a future TODO.
        std::list<Friend> friends; // list of friends -- to be fully implemented as a backend/client/UI feature in the future
        YouTubeData youTubeData; // provided YouTube Data on file upload

    public:
        /**
         * Constructs a User with identity fields.
         * \param userID   Unique user identifier chosen at registration.
         * \param username Display name shown in the UI.
         * \param email    User's email address (may be empty).
         * \param password Plaintext password.
         */
        User(const std::string& userID,
             const std::string& username,
             const std::string& email,
             const std::string& password);

        /** \return Unique user identifier. */
        std::string        getUserID()   const;
        /** \return Display name. */
        std::string        getUsername() const;
        /** \return Email address. */
        std::string        getEmail()    const;
        /** \return Plaintext password. */
        std::string        getPassword() const;
        /** \return List of this user's friends. */
        std::list<Friend>  getFriends()  const;
        /** \return Mutable reference to this user's YouTube data. */
        YouTubeData&       getYouTubeData();
        /** \return Const reference to this user's YouTube data. */
        const YouTubeData& getYouTubeData() const;

        /** \param username New display name. */
        void setUsername(const std::string& username);
        /** \param email New email address. */
        void setEmail(const std::string& email);
        /** \param password New plaintext password. */
        void setPassword(const std::string& password);
        /** \param youTubeData Replaces the user's YouTube data in bulk. */
        void setYouTubeData(const YouTubeData& youTubeData);

        /**
         * Adds a friend entry for the given user.
         * \param user User to add as a friend.
         */
        void addFriend(const User& user);

        /**
         * Removes the friend entry with the given userID.
         * \param userID ID of the friend to remove.
         */
        void removeFriend(const std::string& userID);

        /**
         * String representation of \c User
         * \return \c string
         */
        std::string toString() const;
};

#endif
