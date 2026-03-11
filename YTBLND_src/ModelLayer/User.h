#ifndef USER_H
#define USER_H

#include <string>
#include<list>

#include "Friend.h"
#include "YouTubeData.h"

class User {
    private:
        std::string userID;
        std::string  email;
        std::string passwordHash;
        std::list<Friend> friends;
        YouTubeData youTubeData;

    public:
        User(const std::string& userID,
             const std::string& email,
             const std::string& passwordHash);

        // Getters
        std::string       getUserID()      const;
        std::string       getEmail()       const;
        std::string       getPasswordHash() const;
        std::list<Friend> getFriends()     const;
        YouTubeData&      getYouTubeData();       // non-const: parser writes into it
        const YouTubeData& getYouTubeData() const; // const overload for read-only access

        // Setters
        void setEmail(const std::string& email);
        void setPasswordHash(const std::string& passwordHash);
        void setYouTubeData(const YouTubeData& youTubeData);

        void addFriend(const User& user);
        void removeFriend(const std::string& userID);
};
#endif