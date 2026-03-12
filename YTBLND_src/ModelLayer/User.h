#ifndef USER_H
#define USER_H

#include <string>
#include <list>

#include "Friend.h"
#include "YouTubeData.h"

class User {
    private:
        std::string userID;
        std::string username;
        std::string email;
        std::string password;   // stored as plaintext at this stage
        std::list<Friend> friends;
        YouTubeData youTubeData;

    public:
        User(const std::string& userID,
             const std::string& username,
             const std::string& email,
             const std::string& password);

        // Getters
        std::string        getUserID()   const;
        std::string        getUsername() const;
        std::string        getEmail()    const;
        std::string        getPassword() const;
        std::list<Friend>  getFriends()  const;
        YouTubeData&       getYouTubeData();
        const YouTubeData& getYouTubeData() const;

        // Setters
        void setUsername(const std::string& username);
        void setEmail(const std::string& email);
        void setPassword(const std::string& password);
        void setYouTubeData(const YouTubeData& youTubeData);

        void addFriend(const User& user);
        void removeFriend(const std::string& userID);
};

#endif
