#include "AppController.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ServiceLayer/YouTubeMetadataFetcher.hpp"
#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <set>

static const std::string kYouTubeAPIKey = "AIzaSyBDzH4_T9NSaAh_s59sFYrHtNnKvHmyARM";

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController(const std::string& dbPath): 
    appState(AppState::getInstance()),
    dataManager(std::make_unique<SqliteDataManager>(dbPath)),  
    server_url("http://137.220.58.22:8080"),
    http(server_url),
    isConnected(false)
{
    registerEvents();
    // TODO: HTTP server integration in progress — skip ping if unreachable
    try {
        std::string resp = http.get("/ping");
        if (resp.find("pong") != std::string::npos) {
            std::cout << "[AppState] Successful connection to the server" << std::endl;
            isConnected = true;
        } else {
            std::cout << "[AppState] Server reachable but unexpected response." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[AppState] Server unavailable, running offline: " << e.what() << std::endl;
        isConnected = false;
    }
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

    // Attempt server registration when online; bail on definitive server errors.
    if (isConnected) {
        try {
            const std::string registerResponse = http.post(http.REG_USER, newUser.toString());
            if (!http.wasLastRequestSuccessful(http.P)) {
                if (http.getLastStatusCode() == http.DUPLICATE) {
                    std::cerr << "[AppController] handleRegister: userID '"
                              << idIt->second << "' already exists on server\n";
                    return;
                }
                std::cerr << "[AppController] handleRegister: server error "
                          << http.getLastStatusCode()
                          << " response=" << registerResponse << "\n";
                return;
            }
        } catch (const std::exception& e) {
            std::cerr << "[AppController] handleRegister: HTTP unavailable, registering locally: "
                      << e.what() << "\n";
        }
    }

    // Always persist to local DB — needed for login validation regardless of server state.
    if (!dataManager->createUser(newUser)) {
        std::cerr << "[AppController] handleRegister: userID '"
                  << idIt->second << "' already exists locally\n";
        return;
    }

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

    // TODO: HTTP login integration incomplete — post credentials to server
    // std::stringstream login_json;
    // login_json << "{\"user_id\":\"" << idIt->second << "\",\"password\":\"" << pwIt->second << "\"}";
    // const std::string loginResponse = http.post(http.LOGIN, login_json.str());

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
            enrichIfMissingMetadata(uid, pVideos);
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
    bool enriched = false;
    try {
        watchLater = YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(watchLater);
        enriched = !kYouTubeAPIKey.empty();
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] handleUploadData: metadata fetch failed: "
                  << ex.what() << "\n";
        // Non-fatal — continue with whatever data we have
    }

    // Drop videos the API couldn't find — they are deleted, private, or unavailable
    if (enriched) {
        watchLater.remove_if([](const Video& v) { return v.getTitle().empty(); });
        std::cout << "[AppController] " << watchLater.size()
                  << " videos available after filtering unavailable entries\n";
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
    std::vector<std::string> notFound;
    std::vector<std::string> noData;

    for (const auto& uid : allParticipantIDs) {
        std::optional<User> p = dataManager->findUserByID(uid);
        if (!p.has_value()) {
            std::cerr << "[AppController] handleCreateBlend: user '" << uid << "' not found\n";
            notFound.push_back(uid);
            continue;
        }
        std::list<Video> pVideos = dataManager->loadWatchLater(uid);
        if (pVideos.empty()) {
            noData.push_back(uid);
        } else {
            enrichIfMissingMetadata(uid, pVideos);
            YouTubeData pyd;
            pyd.setWatchLaterVideos(pVideos);
            p->setYouTubeData(pyd);
            participants.push_back(*p);
        }
    }

    // Report users without uploaded data back to the UI via AppState
    // (non-existent users are excluded — they should have been rejected at the UI level)
    appState.setUsersWithoutData(noData);

    if (participants.empty()) {
        std::cerr << "[AppController] handleCreateBlend: no participants have data\n";
        return;
    }

    appState.setIsBlendGenerating(true);

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );

    // Persist the blend and all valid participants (existing users, even those
    // without data yet, so they are found on their next login).
    // Non-existent users (notFound) are excluded — they have no DB row.
    std::vector<std::string> validParticipantIDs;
    for (const auto& uid : allParticipantIDs) {
        bool isNotFound = std::find(notFound.begin(), notFound.end(), uid) != notFound.end();
        if (!isNotFound) validParticipantIDs.push_back(uid);
    }
    User* creator = appState.getCurrentUser();
    std::string creatorID = creator ? creator->getUserID() : validParticipantIDs[0];
    dataManager->saveBlend(currentBlend->getBlendID(), creatorID,
                           currentBlend->getAlgorithmUsed(), validParticipantIDs);

    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);
    appState.setIsBlendGenerating(false);

    std::cout << "[AppController] Blend created with "
              << currentBlend->size() << " videos\n";
}

bool AppController::lookupUser(const std::string& userID) {
    return dataManager->findUserByID(userID).has_value();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void AppController::enrichIfMissingMetadata(const std::string& userID,
                                             std::list<Video>&  videos) {
    bool needsEnrichment = std::any_of(videos.begin(), videos.end(),
        [](const Video& v) { return v.getTitle().empty(); });
    if (!needsEnrichment) return;

    std::cout << "[AppController] enrichIfMissingMetadata: re-enriching "
              << videos.size() << " videos for '" << userID << "'\n";
    try {
        videos = YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(videos);
        // Drop any video still missing a title — confirmed unavailable on YouTube
        if (!kYouTubeAPIKey.empty())
            videos.remove_if([](const Video& v) { return v.getTitle().empty(); });
        dataManager->saveWatchLater(userID, videos);
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] enrichIfMissingMetadata failed for '"
                  << userID << "': " << ex.what() << "\n";
    }
}

void AppController::handlePlayVideo(const EventPayload& payload) {
    // TODO: read "videoID" from payload
    // TODO: open the video — exact mechanism (browser, in-app) TBD
    std::cout << "[AppController] handlePlayVideo called (stub)\n";
}

void AppController::handleRefresh(const EventPayload& /*payload*/) {
    Blend* existing = appState.getActiveBlend();
    if (!existing) {
        std::cerr << "[AppController] handleRefresh: no active blend\n";
        return;
    }

    // Build the set of video IDs already shown in the current blend
    std::set<std::string> shownIDs;
    for (const auto& v : existing->getVideoList())
        shownIDs.insert(v.getVideoID());

    // Reload each participant's full video pool from DB, then exclude shown videos
    std::list<User> participants;
    for (const User& participant : existing->getParticipants()) {
        const std::string& uid = participant.getUserID();
        std::optional<User> p = dataManager->findUserByID(uid);
        if (!p.has_value()) continue;

        std::list<Video> allVideos = dataManager->loadWatchLater(uid);
        if (allVideos.empty()) continue;

        // Re-enrich any videos that are still missing metadata; saves to DB
        enrichIfMissingMetadata(uid, allVideos);

        // Build a fresh pool of videos not yet shown
        std::list<Video> freshPool;
        for (const auto& v : allVideos) {
            if (shownIDs.find(v.getVideoID()) == shownIDs.end())
                freshPool.push_back(v);
        }
        // If all of this user's videos were already shown, reset to the full pool
        if (freshPool.empty()) freshPool = allVideos;

        YouTubeData pyd;
        pyd.setWatchLaterVideos(freshPool);
        p->setYouTubeData(pyd);
        participants.push_back(*p);
    }

    if (participants.empty()) {
        std::cerr << "[AppController] handleRefresh: no participants have data\n";
        return;
    }

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
    );
    appState.setActiveBlend(currentBlend.get());

    std::cout << "[AppController] Blend refreshed with "
              << currentBlend->size() << " new videos\n";
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