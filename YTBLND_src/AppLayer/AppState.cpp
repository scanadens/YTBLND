#include "AppState.hpp"

using namespace std;

AppState::AppState(): 
    currentUser(nullptr), 
    activeBlend(nullptr), 
    isBlendGenerating(false) {}

// -- Singleton -----------------------------------------------------------------

AppState& AppState::getInstance() {
    // Constructed once on first call; guaranteed by C++11 to be thread-safe.
    static AppState instance;
    return instance;
}

// -- Current User --------------------------------------------------------------

User* AppState::getCurrentUser() const {
    return currentUser;
}

void AppState::setCurrentUser(User* user) {
    // If a different user is logging in, the active blend is no longer valid.
    if (currentUser != nullptr && user != nullptr &&
        currentUser->getUserID() != user->getUserID()) {
        activeBlend = nullptr;
        chatRooms.clear();
    }
    currentUser = user;
}

// -- Active Blend --------------------------------------------------------------

Blend* AppState::getActiveBlend() const {
    return activeBlend;
}

void AppState::setActiveBlend(Blend* blend) {
    activeBlend = blend;
}

// -- Blend Generation Flag -----------------------------------------------------

bool AppState::isGeneratingBlend() const {
    return isBlendGenerating;
}

void AppState::setIsBlendGenerating(bool flag) {
    isBlendGenerating = flag;
}

// -- Chat Rooms ----------------------------------------------------------------

ChatRoom* AppState::getActiveChatRoom() {
    if (activeBlend == nullptr) return nullptr;
    return getChatRoom(activeBlend->getBlendID());
}

void AppState::createChatRoom(const Blend& blend) {
    // Build participant ID list from the Blend's User list
    std::vector<std::string> participantIDs;
    for (const auto& user : blend.getParticipants()) {
        participantIDs.push_back(user.getUserID());
    }

    // Insert or replace the room for this blendID
    chatRooms.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(blend.getBlendID()),
        std::forward_as_tuple(blend.getBlendID(), participantIDs)
    );
}

ChatRoom* AppState::getChatRoom(const std::string& blendID) {
    auto it = chatRooms.find(blendID);
    if (it == chatRooms.end()) return nullptr;
    return &it->second;
}

// -- Session Users -------------------------------------------------------------

void AppState::addSessionUser(const User& user) {
    sessionUsers.insert_or_assign(user.getUserID(), user);
}

std::map<std::string, User> AppState::getSessionUsers() const {
    return sessionUsers;
}

void AppState::clearSessionUsers() {
    sessionUsers.clear();
}

// -- Blend creation feedback ---------------------------------------------------

void AppState::setUsersWithoutData(const std::vector<std::string>& users) {
    usersWithoutData = users;
}

std::vector<std::string> AppState::getUsersWithoutData() const {
    return usersWithoutData;
}

void AppState::setPendingBlendMessage(const std::string& msg) {
    pendingBlendMessage = msg;
}

std::string AppState::takePendingBlendMessage() {
    std::string msg = pendingBlendMessage;
    pendingBlendMessage.clear();
    return msg;
}

void AppState::setPendingUploadError(const std::string& msg) {
    pendingUploadError = msg;
}

std::string AppState::takePendingUploadError() {
    std::string msg = pendingUploadError;
    pendingUploadError.clear();
    return msg;
}

// -- Session -------------------------------------------------------------------

void AppState::clearSession() {
    currentUser       = nullptr;
    activeBlend       = nullptr;
    isBlendGenerating = false;
    chatRooms.clear();
    sessionUsers.clear();
    usersWithoutData.clear();
    pendingBlendMessage.clear();
    pendingUploadError.clear();
}