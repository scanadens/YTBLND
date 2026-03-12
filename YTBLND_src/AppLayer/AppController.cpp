#include "AppController.h"
#include "../AppInfrastructure/WatchLaterParser.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.h"
#include <iostream>
#include <list>

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController()
    : appState(AppState::getInstance()),
      dataManager(nullptr) // stub — replace with real instance when implemented
{
    registerEvents();
}

AppController::~AppController() {
    // Persist current user before shutdown when DataManager is implemented:
    // if (dataManager && appState.getCurrentUser())
    //     dataManager->persistUser(*appState.getCurrentUser());

    eventRouter.deregisterAll();
}

// ── Event Registration ────────────────────────────────────────────────────────

void AppController::registerEvents() {
    // Binds each named event to the corresponding handler on this controller.
    // Panels dispatch events by name; EventRouter calls the handler below.
    eventRouter.registerListener("login",       [this](const EventPayload& p){ handleLogin(p); });
    eventRouter.registerListener("logout",      [this](const EventPayload& p){ handleLogout(p); });
    eventRouter.registerListener("uploadData",  [this](const EventPayload& p){ handleUploadData(p); });
    eventRouter.registerListener("createBlend", [this](const EventPayload& p){ handleCreateBlend(p); });
    eventRouter.registerListener("playVideo",   [this](const EventPayload& p){ handlePlayVideo(p); });
    eventRouter.registerListener("refresh",     [this](const EventPayload& p){ handleRefresh(p); });
    eventRouter.registerListener("openChat",    [this](const EventPayload& p){ handleOpenChat(p); });
    eventRouter.registerListener("sendMessage", [this](const EventPayload& p){ handleSendMessage(p); });
}

// ── Getter ────────────────────────────────────────────────────────────────────

EventRouter& AppController::getEventRouter() {
    return eventRouter;
}

// ── Event Handlers ────────────────────────────────────────────────────────────

void AppController::handleLogin(const EventPayload& payload) {
    // TODO: retrieve email and password from payload
    // TODO: call DataManager to load the matching User by email
    // TODO: verify passwordHash
    // TODO: call appState.setCurrentUser() on success
    // TODO: trigger handleRefresh if YouTube data is stale
    std::cout << "[AppController] handleLogin called (stub)\n";
}

void AppController::handleLogout(const EventPayload& payload) {
    // TODO: call dataManager->persistUser() for current user
    // TODO: call appState.clearSession()
    // TODO: instruct MainFrame to navigate to LoginPanel
    std::cout << "[AppController] handleLogout called (stub)\n";
}

void AppController::handleUploadData(const EventPayload& payload) {
    auto fileIt = payload.find("filePath");
    auto userIt = payload.find("userID");
    if (fileIt == payload.end() || userIt == payload.end()) {
        std::cerr << "[AppController] handleUploadData: missing 'filePath' or 'userID' in payload\n";
        return;
    }

    const std::string& filePath = fileIt->second;
    const std::string& userID   = userIt->second;

    std::list<Video> watchLater = WatchLaterParser(filePath).parse();

    User user(userID, "", "");
    YouTubeData yd;
    yd.setWatchLaterVideos(watchLater);
    user.setYouTubeData(yd);

    appState.addSessionUser(user);
    std::cout << "[AppController] Loaded " << watchLater.size()
              << " watch-later videos for user '" << userID << "'\n";
}

void AppController::handleCreateBlend(const EventPayload& payload) {
    std::map<std::string, User> sessionUsers = appState.getSessionUsers();
    if (sessionUsers.empty()) {
        std::cerr << "[AppController] handleCreateBlend: no session users loaded\n";
        return;
    }

    std::list<User> participants;
    for (const auto& entry : sessionUsers) {
        participants.push_back(entry.second);
    }

    appState.setIsBlendGenerating(true);

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    appState.setIsBlendGenerating(false);

    std::cout << "[AppController] Blend created with "
              << currentBlend->size() << " videos\n";
}

void AppController::handlePlayVideo(const EventPayload& payload) {
    // TODO: read "videoID" from payload
    // TODO: open the video — exact mechanism (browser, in-app) TBD
    std::cout << "[AppController] handlePlayVideo called (stub)\n";
}

void AppController::handleRefresh(const EventPayload& payload) {
    // TODO: re-parse or re-fetch YouTube data for the current user
    // TODO: update AppState and notify the active panel
    std::cout << "[AppController] handleRefresh called (stub)\n";
}

void AppController::handleOpenChat(const EventPayload& payload) {
    // TODO: check appState.getActiveChatRoom() is not null
    // TODO: instruct MainFrame to show ChatPanel
    std::cout << "[AppController] handleOpenChat called (stub)\n";
}

void AppController::handleSendMessage(const EventPayload& payload) {
    // TODO: read "userID" and "text" from payload
    // TODO: retrieve appState.getActiveChatRoom()
    // TODO: verify userID is a participant via chatRoom->isParticipant()
    // TODO: call chatRoom->addMessage(userID, text)
    // TODO: notify ChatPanel to re-render the message list
    std::cout << "[AppController] handleSendMessage called (stub)\n";
}