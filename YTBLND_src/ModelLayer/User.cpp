#include "User.h"

User::User(const std::string& userID,
           const std::string& email,
           const std::string& passwordHash)
    : userID(userID), email(email), passwordHash(passwordHash) {}

// Getters 

std::string User::getUserID() const {
    return userID;
}

std::string User::getEmail() const {
    return email;
}

std::string User::getPasswordHash() const {
    return passwordHash;
}

std::list<Friend> User::getFriends() const {
    return friends;
}

YouTubeData& User::getYouTubeData() {
    return youTubeData;
}

const YouTubeData& User::getYouTubeData() const {
    return youTubeData;
}

// Setters 
void User::setEmail(const std::string& email) {
    this->email = email;
}

void User::setPasswordHash(const std::string& passwordHash) {
    this->passwordHash = passwordHash;
}

void User::setYouTubeData(const YouTubeData& youTubeData) {
    this->youTubeData = youTubeData;
}

// Friend Management (implement later)
void User::addFriend(const User& user) {
    // Do not add duplicates
    for (const auto& f : friends) {
        if (f.getUserID() == user.getUserID()) return;
    }
    friends.emplace_back(user.getUserID(), user.getEmail(), user.getEmail());
}

void User::removeFriend(const std::string& userID) {
    friends.remove_if([&userID](const Friend& f) {
        return f.getUserID() == userID;
    });
}
