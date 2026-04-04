/**
 * \file User.cpp
 * \brief Implementation for User.
 * \author Jasmine Kumar
 */

#include "User.hpp"

#include "JsonUtils.hpp"

#include <sstream>

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

std::string User::toString() const {
    std::ostringstream friendsJson;
    friendsJson << "[";
    bool firstFriend = true;
    for (const auto& friendUser : friends) {
        if (!firstFriend) {
            friendsJson << ",";
        }
        friendsJson << friendUser.toString();
        firstFriend = false;
    }
    friendsJson << "]";

    return "{"
           "\"user_id\":" + ModelJson::quote(userID) + ","
           "\"username\":" + ModelJson::quote(username) + ","
           "\"email\":" + ModelJson::quote(email) + ","
           "\"friends\":" + friendsJson.str() + ","
           "\"youtube_data\":" + youTubeData.toString()
           + "}";
}
