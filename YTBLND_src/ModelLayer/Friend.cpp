/**
 * \file Friend.cpp
 * \brief Implementation for Friend.
 * \author Jasmine Kumar
 */

#include "Friend.hpp"
#include "JsonUtils.hpp"

//
Friend::Friend(const std::string& userID,
               const std::string& displayName,
               const std::string& email)
    : userID(userID), displayName(displayName), email(email) {}

// Getters

std::string Friend::getUserID() const {
    return userID;
}

std::string Friend::getDisplayName() const {
    return displayName;
}

std::string Friend::getEmail() const {
    return email;
}

//Setters (userID is intentionally omitted - it is immutable after construction)

void Friend::setDisplayName(const std::string& displayName) {
    this->displayName = displayName;
}

void Friend::setEmail(const std::string& email) {
    this->email = email;
}

std::string Friend::toString() const {
    return "{"
           "\"user_id\":" + ModelJson::quote(userID) + ","
           "\"username\":" + ModelJson::quote(displayName) + ","
           "\"email\":" + ModelJson::quote(email)
           + "}";
}
