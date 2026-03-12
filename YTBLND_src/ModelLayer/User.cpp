#include "User.h"

User::User(const std::string& userID,
           const std::string& username,
           const std::string& email,
           const std::string& password)
    : userID(userID), username(username), email(email), password(password) {}

// Getters

std::string User::getUserID() const   { return userID; }
std::string User::getUsername() const { return username; }
std::string User::getEmail() const    { return email; }
std::string User::getPassword() const { return password; }

std::list<Friend> User::getFriends() const { return friends; }

YouTubeData& User::getYouTubeData()             { return youTubeData; }
const YouTubeData& User::getYouTubeData() const { return youTubeData; }

// Setters

void User::setUsername(const std::string& username) { this->username = username; }
void User::setEmail(const std::string& email)       { this->email = email; }
void User::setPassword(const std::string& password) { this->password = password; }

void User::setYouTubeData(const YouTubeData& youTubeData) {
    this->youTubeData = youTubeData;
}

// Friend management

void User::addFriend(const User& user) {
    for (const auto& f : friends) {
        if (f.getUserID() == user.getUserID()) return; // no duplicates
    }
    friends.emplace_back(user.getUserID(), user.getUsername(), user.getEmail());
}

void User::removeFriend(const std::string& userID) {
    friends.remove_if([&userID](const Friend& f) {
        return f.getUserID() == userID;
    });
}
