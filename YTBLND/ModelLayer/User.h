#ifndef USER_H
#define USER_H

#include <string>
#include<list>

#include "Friend.h"
#include "YouTubeData.h"

class User {
    private:
        std::string userID, email, passwordHash;
        std::list<Friend> friends;
        YouTubeData youTubeData;

    public:
        void addFriend(User user);
        void removeFriend(std::string userID);
};
#endif