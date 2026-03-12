#include "AppController.h"
#include "../AppInfrastructure/WatchLaterParser.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.h"
#include <iostream>
#include <list>

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController()
    : appState(AppState::getInstance()),
      dataManager(std::make_unique<SqliteDataManager>("ytblnd.db"))
{
    registerEvents();
}

AppController::~AppController() {
    eventRouter.deregisterAll();
}

// ── Event Registration ────────────────────────────────────────────────────────

void AppController::registerEvents() {
    // Binds each named event to the corresponding handler on this controller.
    // Panels dispatch events by name; EventRouter calls the handler below.
    eventRouter.registerListener("register",    [this](const EventPayload& p){ handleRegister(p); });
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

void AppController::handleRegister(const EventPayload& payload) {
    auto idIt   = payload.find("userID");
    auto unIt   = payload.find("username");
    auto emIt   = payload.find("email");
    auto pwIt   = payload.find("password");

    if (idIt == payload.end() || unIt == payload.end() ||
        pwIt == payload.end()) {
        std::cerr << "[AppController] handleRegister: missing required fields\n";
        return;
    }

    User newUser(idIt->second,
                 unIt->second,
                 emIt != payload.end() ? emIt->second : "",
                 pwIt->second);

    if (!dataManager->createUser(newUser)) {
        std::cerr << "[AppController] handleRegister: userID '"
                  << idIt->second << "' already exists\n";
        return;
    }

    // Log the new user in immediately after registration
    appState.setCurrentUser(new User(newUser));
    std::cout << "[AppController] Registered and logged in as '"
              << newUser.getUsername() << "'\n";
}

void AppController::handleLogin(const EventPayload& payload) {
    auto idIt = payload.find("userID");
    auto pwIt = payload.find("password");

    if (idIt == payload.end() || pwIt == payload.end()) {
        std::cerr << "[AppController] handleLogin: missing 'userID' or 'password'\n";
        return;
    }

    if (!dataManager->validatePassword(idIt->second, pwIt->second)) {
        std::cerr << "[AppController] handleLogin: invalid credentials for '"
                  << idIt->second << "'\n";
        return;
    }

    std::optional<User> user = dataManager->findUserByID(idIt->second);
    if (!user.has_value()) {
        std::cerr << "[AppController] handleLogin: user not found after validation\n";
        return;
    }

    appState.setCurrentUser(new User(*user));
    std::cout << "[AppController] Logged in as '" << user->getUsername() << "'\n";
}

void AppController::handleLogout(const EventPayload& payload) {
    User* u = appState.getCurrentUser();
    if (u) {
        std::cout << "[AppController] Logging out '" << u->getUsername() << "'\n";
        delete u;
    }
    appState.setCurrentUser(nullptr);
    appState.clearSession();
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

    User user(userID, "", "", "");
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