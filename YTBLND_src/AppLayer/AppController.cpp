#include "AppController.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"

#include <exception>
#include <iostream>
#include <list>
#include <nlohmann/json.hpp>
#include <optional>

using Json = nlohmann::json;

std::optional<User> AppController::parseUserFromAuthResponse(const std::string& response,
                                                             const std::string& fallbackPassword) {
    Json parsed = Json::parse(response, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("user_id")) {
        return std::nullopt;
    }

    const std::string userID = parsed.value("user_id", "");
    if (userID.empty()) {
        return std::nullopt;
    }

    return User(
        userID,
        parsed.value("username", userID),
        parsed.value("email", ""),
        fallbackPassword
    );
}

std::optional<std::string> AppController::parseBlendID(const std::string& response) {
    Json parsed = Json::parse(response, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object()) {
        return std::nullopt;
    }

    const std::string blendID = parsed.value("blend_id", "");
    if (blendID.empty()) {
        return std::nullopt;
    }

    return blendID;
}

std::vector<std::string> AppController::parseParticipantIDs(const std::string& response) {
    std::vector<std::string> participantIDs;

    Json parsed = Json::parse(response, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("participants")) {
        return participantIDs;
    }

    if (!parsed["participants"].is_array()) {
        return participantIDs;
    }

    for (const auto& participant : parsed["participants"]) {
        if (participant.is_string()) {
            participantIDs.push_back(participant.get<std::string>());
        }
    }

    return participantIDs;
}

std::list<Video> AppController::parseWatchLaterVideos(const std::string& response) {
    std::list<Video> videos;

    Json parsed = Json::parse(response, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("videos")) {
        return videos;
    }

    if (!parsed["videos"].is_array()) {
        return videos;
    }

    for (const auto& item : parsed["videos"]) {
        if (!item.is_object()) {
            continue;
        }

        const std::string videoID = item.value("video_id", "");
        if (videoID.empty()) {
            continue;
        }

        videos.emplace_back(
            videoID,
            item.value("title", ""),
            item.value("channel_id", ""),
            item.value("thumbnail_url", ""),
            item.value("duration", 0),
            std::list<std::string>{}
        );
    }

    return videos;
}

std::list<User> AppController::loadParticipantsWithWatchLater(
    const std::vector<std::string>& participantIDs,
    std::vector<std::string>* missingData) {
    std::list<User> participants;

    for (const auto& uid : participantIDs) {
        const std::string watchLaterResponse = http.get(http.build_watch_later_endpoint(uid));
        if (!http.wasLastRequestSuccessful(http.G)) {
            if (missingData != nullptr) {
                missingData->push_back(uid);
            }
            continue;
        }

        std::list<Video> watchLater = parseWatchLaterVideos(watchLaterResponse);
        if (watchLater.empty()) {
            if (missingData != nullptr) {
                missingData->push_back(uid);
            }
            continue;
        }

        User user(uid, uid, "", "");
        YouTubeData yt;
        yt.setWatchLaterVideos(watchLater);
        user.setYouTubeData(yt);
        participants.push_back(user);
    }

    return participants;
}

using namespace std;

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController(const std::string& dbPath): 
    appState(AppState::getInstance()),
    server_url("http://137.220.58.22:8080"), 
    http(server_url),
    isConnected(false)
{
    (void)dbPath;
    registerEvents();
    // performing http health check to ensure connection was successull
    string resp = http.get("/ping"); //grab the server response
    if (resp.find("pong") != std::string::npos) {
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

    const string registerResponse = http.post(
        http.REG_USER,
        RequestJsonBuilder::buildRegisterJson(
            newUser.getUserID(),
            newUser.getUsername(),
            newUser.getEmail(),
            newUser.getPassword()
        )
    );
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

    const auto loginResponse = http.post(
        http.LOGIN,
        RequestJsonBuilder::buildLoginJson(idIt->second, pwIt->second)
    );

    // retrieve the status of the request 
    const bool req_status = http.isRequestSuccessful(http.P, http.getLastStatusCode());

    if (!req_status) {
        std::cerr << "[AppController] handleLogin: invalid credentials or '"
                  << idIt->second 
                  << "' does not exist..."
                  << endl;
        return;
    }

    std::optional<User> user = parseUserFromAuthResponse(loginResponse, pwIt->second);
    if (!user.has_value()) {
        std::cerr << "[AppController] handleLogin: malformed login response\n";
        return;
    }

    // Restore persisted watch-later data from the backend
    std::list<Video> watchLater;
    const std::string watchLaterResponse = http.get(http.build_watch_later_endpoint(idIt->second));
    if (http.wasLastRequestSuccessful(http.G)) {
        watchLater = parseWatchLaterVideos(watchLaterResponse);
    }

    if (!watchLater.empty()) {
        YouTubeData yd;
        yd.setWatchLaterVideos(watchLater);
        user->setYouTubeData(yd);
        appState.addSessionUser(*user);
    }

    appState.setCurrentUser(new User(*user));
    std::cout << "[AppController] Logged in as '" << user->getUsername() << "'\n";

    // Check if this user belongs to a previously saved blend
    const std::string latestBlendResponse = http.get(http.build_latest_blend_endpoint(idIt->second));
    if (!http.wasLastRequestSuccessful(http.G)) {
        return;
    }
    std::optional<std::string> blendID = parseBlendID(latestBlendResponse);
    if (!blendID.has_value()) {
        return;
    }

    const std::string participantResponse = http.get(http.build_blend_participant_endpoint(*blendID));
    if (!http.wasLastRequestSuccessful(http.G)) {
        return;
    }

    const std::vector<std::string> participantIDs = parseParticipantIDs(participantResponse);
    std::list<User> participants = loadParticipantsWithWatchLater(participantIDs, nullptr);

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

    const std::string saveWatchLaterResponse =
        http.post(
            http.build_watch_later_endpoint(userID),
            RequestJsonBuilder::buildWatchLaterJson(watchLater)
        );
    if (!http.wasLastRequestSuccessful(http.P)) {
        std::cerr << "[AppController] handleUploadData: failed to persist watch-later list, HTTP "
                  << http.getLastStatusCode() << " response=" << saveWatchLaterResponse << "\n";
        return;
    }

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
    // so their data is included and the blend stays up to date.
    const std::string latestBlendResponse = http.get(http.build_latest_blend_endpoint(userID));
    if (!http.wasLastRequestSuccessful(http.G)) {
        return;
    }

    std::optional<std::string> blendID = parseBlendID(latestBlendResponse);
    if (!blendID.has_value()) {
        return;
    }

    const std::string participantResponse = http.get(http.build_blend_participant_endpoint(*blendID));
    if (!http.wasLastRequestSuccessful(http.G)) {
        return;
    }

    const std::vector<std::string> participantIDs = parseParticipantIDs(participantResponse);
    std::list<User> participants = loadParticipantsWithWatchLater(participantIDs, nullptr);

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

    std::vector<std::string> missingData;
    std::list<User> participants =
        loadParticipantsWithWatchLater(allParticipantIDs, &missingData);

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
    const std::string createBlendResponse =
        http.post(http.BLEND,
                  RequestJsonBuilder::buildBlendJson(currentBlend->getBlendID(),
                                                     creatorID,
                                                     currentBlend->getAlgorithmUsed(),
                                                     allParticipantIDs));
    if (!http.wasLastRequestSuccessful(http.P)) {
        appState.setIsBlendGenerating(false);
        std::cerr << "[AppController] handleCreateBlend: failed to persist blend, HTTP "
                  << http.getLastStatusCode() << " response=" << createBlendResponse << "\n";
        return;
    }

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