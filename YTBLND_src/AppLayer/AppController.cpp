#include "AppController.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ServiceLayer/YouTubeMetadataFetcher.hpp"
#include <exception>
#include <iostream>
#include <list>
#include <sstream>

using namespace std;

static const std::string kYouTubeAPIKey = "AIzaSyBDzH4_T9NSaAh_s59sFYrHtNnKvHmyARM";

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController(const std::string& dbPath): 
    appState(AppState::getInstance()),
    dataManager(std::make_unique<SqliteDataManager>(dbPath)),  
    server_url("http://<137.220.58.22>:8080"), 
    http(server_url),
    isConnected(false)
{
    registerEvents();
    // performing http health check to ensure connection was successull
    string resp = http.get("/ping"); //grab the server response
    if (resp.find("pong")) {
        cout << "[AppState] Successful connection to the server" << endl;
        isConnected = true;
    } else {
        cout << "[AppState] Was unable to connect to the server. Try checking internet connection and restart the app." << endl;
        isConnected = false;
    }

    //TODO: add some sort of counter-measure or alternative behaviour if theres no connection
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
    // verifying user data structure
    auto idIt   = payload.find("userID");
    auto unIt   = payload.find("username");
    auto emIt   = payload.find("email");
    auto pwIt   = payload.find("password");

    if (idIt == payload.end() || unIt == payload.end() ||
        pwIt == payload.end()) { // if user data is malformed, notify
        std::cerr << "[AppController] handleRegister: missing required fields\n";
        return;
    }

    // building user data structure
    User newUser(idIt->second,
                 unIt->second,
                 emIt != payload.end() ? emIt->second : "",
                 pwIt->second);

    const string registerResponse = http.post(http.REG_USER, newUser.toString());
    if (!http.wasLastRequestSuccessful(http.P)) {
        if (http.getLastStatusCode() == http.DUPLICATE) {
            cerr << "[AppController] handleRegister: userID '"
                 << idIt->second 
                 << "' already exists"
                 << endl;
            return;
        }

        cerr << "[AppController] handleRegister: server rejected registration with HTTP "
             << http.getLastStatusCode() 
             << " response=" 
             << registerResponse 
             << endl;
        return;
    }

    // Log the new user in immediately after registration
    appState.setCurrentUser(new User(newUser));
    std::cout << "[AppController] Registered and logged in as '"
              << newUser.getUsername() << "'\n";
}

void AppController::handleLogin(const EventPayload& payload) {
    // verifing incorrect userID and password combination
    auto idIt = payload.find("userID");
    auto pwIt = payload.find("password");

    if (idIt == payload.end() || pwIt == payload.end()) {
        std::cerr << "[AppController] handleLogin: missing 'userID' or 'password'\n";
        return;
    }

    stringstream login_json; 
    login_json << "{\"user_id\":" << // TODO: left off here

    const string loginResponse = http.post(http.LOGIN, )

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

    // Restore persisted Watch Later data onto the user object
    std::list<Video> watchLater = dataManager->loadWatchLater(idIt->second);
    if (!watchLater.empty()) {
        YouTubeData yd;
        yd.setWatchLaterVideos(watchLater);
        user->setYouTubeData(yd);
        appState.addSessionUser(*user);
    }

    appState.setCurrentUser(new User(*user));
    std::cout << "[AppController] Logged in as '" << user->getUsername() << "'\n";

    // Check if this user belongs to a previously saved blend
    std::optional<std::string> blendID = dataManager->findBlendForUser(idIt->second);
    if (!blendID.has_value()) return;

    // Load all participants and rebuild their YouTubeData from the DB
    std::vector<std::string> participantIDs = dataManager->loadBlendParticipants(*blendID);
    std::list<User> participants;
    for (const auto& uid : participantIDs) {
        std::optional<User> p = dataManager->findUserByID(uid);
        if (!p.has_value()) continue;
        std::list<Video> pVideos = dataManager->loadWatchLater(uid);
        if (!pVideos.empty()) {
            YouTubeData pyd;
            pyd.setWatchLaterVideos(pVideos);
            p->setYouTubeData(pyd);
            participants.push_back(*p);
        }
    }

    if (participants.empty()) return; // no one has data yet — nothing to show

    // Re-run the algorithm to produce a fresh blend from current persisted data
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    // If this user has no data, leave a message for LoginPanel to display
    if (watchLater.empty()) {
        appState.setPendingBlendMessage(
            "You have been added to a blend. Upload your Watch Later "
            "data to contribute your videos to it.");
    }

    std::cout << "[AppController] Restored blend '" << *blendID
              << "' with " << currentBlend->size() << " videos\n";
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

    std::list<Video> watchLater;
    try {
        watchLater = YouTubeDataImporter().import(filePath);
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] handleUploadData: import failed for '"
                  << filePath << "' - " << ex.what() << "\n";
        return;
    }

    // Enrich with title, channel name, thumbnail URL, and channel logo via the API
    std::cout << "[AppController] Fetching YouTube metadata for "
              << watchLater.size() << " videos...\n";
    try {
        watchLater = YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(watchLater);
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] handleUploadData: metadata fetch failed: "
                  << ex.what() << "\n";
        // Non-fatal — continue with whatever data we have
    }

    // Persist the enriched videos to the DB so they survive logout
    dataManager->saveWatchLater(userID, watchLater);

    // Update the current user's in-memory YouTubeData
    User* currentUser = appState.getCurrentUser();
    if (currentUser && currentUser->getUserID() == userID) {
        YouTubeData yd;
        yd.setWatchLaterVideos(watchLater);
        currentUser->setYouTubeData(yd);
        appState.addSessionUser(*currentUser);
    }

    std::cout << "[AppController] Saved " << watchLater.size()
              << " watch-later videos for user '" << userID << "'\n";

    // If this user is already a blend participant, re-run the algorithm
    // so their data is included and the blend stays up to date
    std::optional<std::string> blendID = dataManager->findBlendForUser(userID);
    if (!blendID.has_value()) return;

    std::vector<std::string> participantIDs = dataManager->loadBlendParticipants(*blendID);
    std::list<User> participants;
    for (const auto& uid : participantIDs) {
        std::optional<User> p = dataManager->findUserByID(uid);
        if (!p.has_value()) continue;
        std::list<Video> pVideos = dataManager->loadWatchLater(uid);
        if (!pVideos.empty()) {
            YouTubeData pyd;
            pyd.setWatchLaterVideos(pVideos);
            p->setYouTubeData(pyd);
            participants.push_back(*p);
        }
    }

    if (participants.empty()) return;

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    std::cout << "[AppController] Re-generated blend '" << *blendID
              << "' after data upload — now " << currentBlend->size() << " videos\n";
}

void AppController::handleCreateBlend(const EventPayload& payload) {
    // Collect participant userIDs from the payload (userID_0, userID_1, ...)
    std::vector<std::string> allParticipantIDs;
    for (int i = 0; ; ++i) {
        auto it = payload.find("userID_" + std::to_string(i));
        if (it == payload.end()) break;
        allParticipantIDs.push_back(it->second);
    }

    if (allParticipantIDs.empty()) {
        std::cerr << "[AppController] handleCreateBlend: no participant IDs in payload\n";
        return;
    }

    // For each participant, load their data from the DB
    std::list<User> participants;
    std::vector<std::string> missingData;

    for (const auto& uid : allParticipantIDs) {
        std::optional<User> p = dataManager->findUserByID(uid);
        if (!p.has_value()) {
            std::cerr << "[AppController] handleCreateBlend: user '" << uid << "' not found\n";
            missingData.push_back(uid);
            continue;
        }
        std::list<Video> pVideos = dataManager->loadWatchLater(uid);
        if (pVideos.empty()) {
            missingData.push_back(uid);
        } else {
            YouTubeData pyd;
            pyd.setWatchLaterVideos(pVideos);
            p->setYouTubeData(pyd);
            participants.push_back(*p);
        }
    }

    // Report missing-data users back to the UI via AppState
    appState.setUsersWithoutData(missingData);

    if (participants.empty()) {
        std::cerr << "[AppController] handleCreateBlend: no participants have data\n";
        return;
    }

    appState.setIsBlendGenerating(true);

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );

    // Persist the blend and all requested participants (including those without
    // data yet, so they are found on their next login)
    User* creator = appState.getCurrentUser();
    std::string creatorID = creator ? creator->getUserID() : allParticipantIDs[0];
    dataManager->saveBlend(currentBlend->getBlendID(), creatorID,
                           currentBlend->getAlgorithmUsed(), allParticipantIDs);

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