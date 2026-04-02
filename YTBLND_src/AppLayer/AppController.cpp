#include "AppController.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ModelLayer/JsonUtils.hpp"
#include "../ServiceLayer/YouTubeMetadataFetcher.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"
#include <algorithm>
#include <ctime>
#include <exception>
#include <iostream>
#include <list>
#include <set>
#include <nlohmann/json.hpp>
#include <optional>

using Json = nlohmann::json;
using namespace std;

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
            std::list<std::string>{},
            item.value("channel_name", ""),
            item.value("channel_logo_url", "")
        );
    }

    return videos;
}

std::list<User> AppController::loadParticipantsWithWatchLater(const std::vector<std::string>& participantIDs, std::vector<std::string>* missingData) {
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

        // enrich any missing metadata
        enrichIfMissingMetadata(uid, watchLater);

        // create a local representation of the user
        User user(uid, uid, "", "");
        YouTubeData yt;
        yt.setWatchLaterVideos(watchLater);
        user.setYouTubeData(yt);
        participants.push_back(user);
    }

    return participants;
}

static const std::string kYouTubeAPIKey = "AIzaSyBDzH4_T9NSaAh_s59sFYrHtNnKvHmyARM";

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController(): 
    appState(AppState::getInstance()),
    server_url("http://137.220.58.22:8080"), 
    http(server_url),
    isConnected(false) {
    registerEvents();

    std::string resp = http.get("/ping");
    try {
        if (resp.find("pong") != std::string::npos) {
            std::cout << "[AppState] Successful connection to the server" << std::endl;
            isConnected = true;
        } else {
            std::cout << "[AppState] Server reachable but unexpected response..." 
            << resp
            << " "
            << endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[AppState] Server unavailable, running offline: " << e.what() << std::endl;
        cout << "[AppState] Server response: " << resp << endl;
    }
}

AppController::AppController(const std::string& /*dbPath*/)
    : AppController() {}

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
    eventRouter.registerListener("deleteAccount", [this](const EventPayload& p){ handleDeleteAccount(p); });
    eventRouter.registerListener("uploadData",  [this](const EventPayload& p){ handleUploadData(p); });
    eventRouter.registerListener("createBlend", [this](const EventPayload& p){ handleCreateBlend(p); });
    eventRouter.registerListener("playVideo",   [this](const EventPayload& p){ handlePlayVideo(p); });
    eventRouter.registerListener("refresh",     [this](const EventPayload& p){ handleRefresh(p); });
    eventRouter.registerListener("openChat",    [this](const EventPayload& p){ handleOpenChat(p); });
    eventRouter.registerListener("sendMessage", [this](const EventPayload& p){ handleSendMessage(p); });
    eventRouter.registerListener("leaveBlend",  [this](const EventPayload& p){ handleLeaveBlend(p); });
    eventRouter.registerListener("selectBlend", [this](const EventPayload& p){ handleSelectBlend(p); });
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

    string registerResponse;

    /**
     * attempt http request. if failed, user will not be able to persist 
     * further into the application
     */
    try {
        if (isConnected) { // if connected to the server, attempt user registration
            string registerResponse = http.post(
                http.REG_USER,
                RequestJsonBuilder::buildRegisterJson(
                    newUser.getUserID(),
                    newUser.getUsername(),
                    newUser.getEmail(),
                    newUser.getPassword()
                )
            );
        }

        if (!http.wasLastRequestSuccessful(http.P)) { // validate request
            if (http.getLastStatusCode() == http.DUPLICATE) {
                cerr << "[AppController] handleRegister: userID '"
                    << idIt->second 
                    << "' already exists"
                    << endl;
                return;
            }
        }
    } catch (const std::exception& e) { // catch any unexpected errors
        std::cerr << "[AppController] handleRegister: HTTP error, unable to move forward: "
            << e.what() << "\n";
        cout << "Server response: " 
            << registerResponse
            << endl;

        return;
    }

    // update this states current user    
    appState.setCurrentUser(new User(newUser));
    std::cout << "[AppController] Registered and logged in as '"
              << newUser.getUsername() << "'\n";

    return;
}

void AppController::handleLogin(const EventPayload& payload) {
    // verifing incorrect userID and password
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
        cout << "[AppController] handleLogin: invalid credentials or '"
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

    string watchLaterResponse;
    // Restore persisted watch-later data from the backend
    list<Video> watchLater;
    try {
        watchLaterResponse = http.get(http.build_watch_later_endpoint(idIt->second));
    } catch (exception& e) {
        std::cerr << "[AppController] handleRegister: HTTP error, unable to move forward: "
            << e.what() << "\n";
        cout << "Server response: " 
            << watchLaterResponse
            << endl;

        return; 
    }

    if (http.wasLastRequestSuccessful(http.G)) {
        watchLater = parseWatchLaterVideos(watchLaterResponse);
        enrichIfMissingMetadata(idIt->second, watchLater);
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

    // Try to recover the blend title from the server
    std::string restoredTitle;
    try {
        const std::string blendsResp = http.get(http.build_user_blends_endpoint(idIt->second));
        if (http.wasLastRequestSuccessful(http.G)) {
            Json blendsParsed = Json::parse(blendsResp, nullptr, false);
            if (!blendsParsed.is_discarded() && blendsParsed.contains("blends") && blendsParsed["blends"].is_array()) {
                for (const auto& item : blendsParsed["blends"]) {
                    if (item.value("blend_id", "") == *blendID) {
                        restoredTitle = item.value("title", "");
                        break;
                    }
                }
            }
        }
    } catch (...) {}

    // Re-run the algorithm to produce a fresh blend from current persisted data
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, restoredTitle)
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

void AppController::handleDeleteAccount(const EventPayload& payload) {
    auto userIt = payload.find("userID");
    auto pwIt = payload.find("password");

    if (userIt == payload.end() || pwIt == payload.end()) {
        std::cerr << "[AppController] handleDeleteAccount: missing 'userID' or 'password'\n";
        return;
    }

    if (!isConnected) {
        std::cerr << "[AppController] handleDeleteAccount: server unavailable in offline mode\n";
        return;
    }

    try {
        const std::string response = http.del(
            http.build_delete_user_endpoint(userIt->second),
            RequestJsonBuilder::buildDeleteAccountJson(pwIt->second)
        );

        if (!http.wasLastRequestSuccessful(http.D)) {
            std::cerr << "[AppController] handleDeleteAccount: delete failed, HTTP "
                      << http.getLastStatusCode() << " response=" << response << "\n";
            return;
        }

        User* currentUser = appState.getCurrentUser();
        if (currentUser != nullptr && currentUser->getUserID() == userIt->second) {
            handleLogout({});
        }

        std::cout << "[AppController] handleDeleteAccount: deleted user '"
                  << userIt->second << "'\n";

    } catch (const std::exception& e) {
        std::cerr << "[AppController] handleDeleteAccount: HTTP failed - "
                  << e.what() << "\n";
    }
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

    // attempt to post data to the server
    std::string saveWatchLaterResponse;
    try {
        // send the request and collect the response
        saveWatchLaterResponse = 
            http.post(
                http.build_watch_later_endpoint(userID),
                RequestJsonBuilder::buildWatchLaterJson(watchLater)
            );
        // verify response from request
        if (!http.wasLastRequestSuccessful(http.P)) { // if response was not successfull
            std::cerr << "[AppController] handleUploadData: failed to persist watch-later list, HTTP "
                    << http.getLastStatusCode() << " response=" << saveWatchLaterResponse << "\n";
            return;
        }

        cout << "[AppController] handleUploadData: Successful upload" << endl;
    } catch (exception& e) {
        std::cerr << "[AppController] handleUploadData: upload failed." << endl;
        cerr << "[AppController] handleUploadData - server response: " << saveWatchLaterResponse << endl;
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

    // Preserve existing blend title if available
    std::string existingTitle;
    Blend* active = appState.getActiveBlend();
    if (active) existingTitle = active->getTitle();

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, existingTitle)
    );
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    std::cout << "[AppController] Re-generated blend '" << *blendID
              << "' after data upload — now " << currentBlend->size() << " videos\n";
}

void AppController::handleCreateBlend(const EventPayload& payload) {
    // Extract blend title
    auto titleIt = payload.find("blendTitle");
    std::string blendTitle = (titleIt != payload.end()) ? titleIt->second : "";

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

    // Report users without uploaded data back to the UI via AppState
    // (non-existent users are excluded — they should have been rejected at the UI level)
    appState.setUsersWithoutData(missingData);

    // Enforce upload: if any participant is missing Watch Later data, refuse to create the blend.
    if (!missingData.empty()) {
        appState.setIsBlendGenerating(false);
        std::cerr << "[AppController] handleCreateBlend: cannot create blend — the following participants have not uploaded Watch Later data:\n";
        for (const auto& uid : missingData) std::cerr << "  " << uid << "\n";
        // UI should show users without data via appState; do not proceed.
        return;
    }

    if (participants.empty()) {
        std::cerr << "[AppController] handleCreateBlend: no participants have data\n";
        return;
    }

    appState.setIsBlendGenerating(true);

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, blendTitle)
    );

    // Persist the blend and all valid participants (existing users, even those
    // without data yet, so they are found on their next login).
    // Non-existent users (notFound) are excluded — they have no DB row.
    std::vector<std::string> validParticipantIDs;
    for (const auto& uid : allParticipantIDs) {
        if (lookupUser(uid)) {
            validParticipantIDs.push_back(uid);
        }
    }

    if (validParticipantIDs.empty()) {
        appState.setIsBlendGenerating(false);
        std::cerr << "[AppController] handleCreateBlend: no valid participants found\n";
        return;
    }

    User* creator = appState.getCurrentUser();
    std::string creatorID = creator ? creator->getUserID() : allParticipantIDs[0];
    const std::string createBlendResponse =
        http.post(http.BLEND,
                  RequestJsonBuilder::buildBlendJson(currentBlend->getBlendID(),
                                                     currentBlend->getTitle(),
                                                     creatorID,
                                                     currentBlend->getAlgorithmUsed(),
                                                     validParticipantIDs));
    if (!http.wasLastRequestSuccessful(http.P)) {
        appState.setIsBlendGenerating(false);
        std::cerr << "[AppController] handleCreateBlend: failed to persist blend, HTTP "
                  << http.getLastStatusCode() << " response=" << createBlendResponse << "\n";
        return;
    }

    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);
    appState.setIsBlendGenerating(false);

    // Register with server so the chat room is provisioned there too.
    registerBlendOnServer(*currentBlend, creatorID);

    std::cout << "[AppController] Blend created with "
              << currentBlend->size() << " videos\n";
}

bool AppController::lookupUser(const std::string& userID) {
    try {
        http.get(http.build_watch_later_endpoint(userID));
        return http.wasLastRequestSuccessful(http.G);
    } catch (const std::exception&) {
        return false;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool AppController::registerBlendOnServer(const Blend& blend,
                                          const std::string& creatorID) {
    if (!isConnected) return false;

    // Build participant array: ["id1","id2",...]
    std::string participantsJson = "[";
    bool first = true;
    for (const User& p : blend.getParticipants()) {
        if (!first) participantsJson += ",";
        participantsJson += ModelJson::quote(p.getUserID());
        first = false;
    }
    participantsJson += "]";

    const std::string body =
        "{\"blend_id\":"    + ModelJson::quote(blend.getBlendID())       + ","
        "\"title\":"        + ModelJson::quote(blend.getTitle())          + ","
        "\"creator_id\":"   + ModelJson::quote(creatorID)                 + ","
        "\"algorithm\":"    + ModelJson::quote(blend.getAlgorithmUsed())  + ","
        "\"participants\":" + participantsJson + "}";

    try {
        http.post(http.BLEND, body);
        if (http.wasLastRequestSuccessful(http.P)) {
            std::cout << "[AppController] registerBlendOnServer: blend '"
                      << blend.getBlendID() << "' registered\n";
            return true;
        }
        // 409 Conflict means it already exists — treat as success.
        if (http.getLastStatusCode() == http.DUPLICATE) {
            std::cout << "[AppController] registerBlendOnServer: blend '"
                      << blend.getBlendID() << "' already on server\n";
            return true;
        }
        std::cerr << "[AppController] registerBlendOnServer: server returned "
                  << http.getLastStatusCode() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[AppController] registerBlendOnServer: HTTP failed — "
                  << e.what() << "\n";
    }
    return false;
}

void AppController::enrichIfMissingMetadata(const std::string& userID,
                                             std::list<Video>&  videos) {
    bool needsEnrichment = std::any_of(videos.begin(), videos.end(),
        [](const Video& v) {
            return v.getTitle().empty() ||
                   v.getChannelID().empty() ||
                   v.getChannelName().empty() ||
                   v.getThumbnailURL().empty() ||
                   v.getChannelLogoURL().empty();
        });
    if (!needsEnrichment) return;

    std::cout << "[AppController] enrichIfMissingMetadata: re-enriching "
              << videos.size() << " videos for '" << userID << "'\n";
    try {
        videos = YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(videos);
        // Drop any video still missing a title — confirmed unavailable on YouTube
        if (!kYouTubeAPIKey.empty())
            videos.remove_if([](const Video& v) { return v.getTitle().empty(); });

        const std::string saveResponse =
            http.post(http.build_watch_later_endpoint(userID),
                      RequestJsonBuilder::buildWatchLaterJson(videos));
        if (!http.wasLastRequestSuccessful(http.P)) {
            std::cerr << "[AppController] enrichIfMissingMetadata: failed to persist refreshed metadata for '"
                      << userID << "', HTTP " << http.getLastStatusCode()
                      << " response=" << saveResponse << "\n";
        }
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

    // Reload each participant's full video pool from the backend, then exclude shown videos
    std::vector<std::string> participantIDs;
    for (const User& participant : existing->getParticipants()) {
        participantIDs.push_back(participant.getUserID());
    }

    // Lightweight load for refresh: fetch each participant's watch-later list
    // but DO NOT call enrichIfMissingMetadata (that can trigger slow network calls).
    std::list<User> participants;
    for (const auto& uid : participantIDs) {
        const std::string watchLaterResponse = http.get(http.build_watch_later_endpoint(uid));
        if (!http.wasLastRequestSuccessful(http.G)) continue;

        std::list<Video> allVideos = parseWatchLaterVideos(watchLaterResponse);
        if (allVideos.empty()) continue;

        // Build a fresh pool of videos not yet shown
        std::list<Video> freshPool;
        for (const auto& v : allVideos) {
            if (shownIDs.find(v.getVideoID()) == shownIDs.end()) {
                freshPool.push_back(v);
            }
        }
        // If all of this user's videos were already shown, reset to the full pool
        if (freshPool.empty()) {
            freshPool = allVideos;
        }

        YouTubeData pyd;
        pyd.setWatchLaterVideos(freshPool);
        User p(uid, uid, "", "");
        p.setYouTubeData(pyd);
        participants.push_back(p);
    }

    if (participants.empty()) {
        std::cerr << "[AppController] handleRefresh: no participants have data\n";
        return;
    }

    std::string refreshTitle = existing->getTitle();
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, refreshTitle)
    );

    // Prevent duplicate videos within the same refreshed blend payload.
    std::set<std::string> seenVideoIDs;
    std::list<Video> uniqueVideos;
    for (const auto& video : currentBlend->getVideoList()) {
        if (seenVideoIDs.insert(video.getVideoID()).second) {
            uniqueVideos.push_back(video);
        }
    }
    currentBlend->setVideoList(uniqueVideos);

    appState.setActiveBlend(currentBlend.get());

    std::cout << "[AppController] Blend refreshed with "
              << currentBlend->size() << " new videos\n";
}

void AppController::handleOpenChat(const EventPayload& payload) {
    auto blendIt = payload.find("blendID");
    auto userIt  = payload.find("userID");
    if (blendIt == payload.end() || userIt == payload.end()) {
        std::cerr << "[AppController] handleOpenChat: missing 'blendID' or 'userID'\n";
        return;
    }

    ChatRoom* room = appState.getActiveChatRoom();
    if (!room) {
        std::cerr << "[AppController] handleOpenChat: no active ChatRoom\n";
        return;
    }

    if (!isConnected) {
        std::cout << "[AppController] handleOpenChat: offline — skipping history fetch\n";
        return;
    }

    try {
        const std::string endpoint =
            http.build_chat_history_endpoint(blendIt->second, userIt->second);
        std::string response = http.get(endpoint);

        if (http.getLastStatusCode() == http.MISS_RM_CNTXT) {
            // Blend not registered on server yet — provision it now and retry.
            Blend* blend = appState.getActiveBlend();
            if (blend) {
                User* creator = appState.getCurrentUser();
                const std::string creatorID = creator ? creator->getUserID() : userIt->second;
                if (registerBlendOnServer(*blend, creatorID)) {
                    response = http.get(endpoint);
                }
            }
        }

        if (!http.wasLastRequestSuccessful(http.G)) {
            std::cerr << "[AppController] handleOpenChat: server returned "
                      << http.getLastStatusCode() << " — leaving ChatRoom empty\n";
            return;
        }

        std::list<Message> history = parseChatHistory(response);
        room->clearMessages();
        for (const Message& msg : history) {
            room->addMessage(msg);
        }
        std::cout << "[AppController] handleOpenChat: loaded "
                  << history.size() << " messages for blend '"
                  << blendIt->second << "'\n";

    } catch (const std::exception& e) {
        std::cerr << "[AppController] handleOpenChat: HTTP failed — "
                  << e.what() << "\n";
    }
}

std::list<Message> AppController::parseChatHistory(const std::string& response) {
    // Extracts a JSON string value from a flat object: "key":"value"
    auto extractStr = [](const std::string& obj, const std::string& key) -> std::string {
        const std::string needle = "\"" + key + "\":\"";
        auto pos = obj.find(needle);
        if (pos == std::string::npos) return "";
        pos += needle.size();
        const auto end = obj.find('"', pos);
        return (end == std::string::npos) ? "" : obj.substr(pos, end - pos);
    };

    std::list<Message> result;

    const std::string arrayKey = "\"messages\":[";
    auto start = response.find(arrayKey);
    if (start == std::string::npos) return result;
    start += arrayKey.size();

    const auto arrEnd = response.find(']', start);
    if (arrEnd == std::string::npos) return result;

    const std::string arr = response.substr(start, arrEnd - start);
    size_t pos = 0;
    while (pos < arr.size()) {
        const auto objStart = arr.find('{', pos);
        if (objStart == std::string::npos) break;
        const auto objEnd = arr.find('}', objStart);
        if (objEnd == std::string::npos) break;

        const std::string obj = arr.substr(objStart, objEnd - objStart + 1);

        const std::string sender  = extractStr(obj, "sender_id");
        const std::string content = extractStr(obj, "content");
        const std::string sentAt  = extractStr(obj, "sent_at");

        if (!sender.empty() && !content.empty()) {
            std::time_t ts = std::time(nullptr);
            if (!sentAt.empty()) {
                std::tm tm{};
                if (strptime(sentAt.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm) != nullptr) {
                    ts = timegm(&tm);
                }
            }
            result.emplace_back(sender, content, ts);
        }

        pos = objEnd + 1;
    }

    return result;
}

void AppController::handleSendMessage(const EventPayload& payload) {
    auto idIt   = payload.find("userID");
    auto textIt = payload.find("text");
    if (idIt == payload.end() || textIt == payload.end()) return;

    ChatRoom* room = appState.getActiveChatRoom();
    if (!room) return;

    if (!room->isParticipant(idIt->second)) {
        std::cerr << "[AppController] handleSendMessage: '"
                  << idIt->second << "' is not a participant\n";
        return;
    }

    room->addMessage(idIt->second, textIt->second);
}

const User* AppController::get_current_user() {
    return appState.getCurrentUser();
}

std::string AppController::get_current_username() {
    if (get_current_user() == nullptr) return "";

    return appState.getCurrentUser()->getUsername();
}

std::string AppController::get_current_email() {
    if (get_current_user() == nullptr) return "";

    return appState.getCurrentUser()->getEmail();
}

std::vector<AppController::BlendSummary> AppController::fetchUserBlends() {
    std::vector<BlendSummary> blends;
    User* user = appState.getCurrentUser();
    if (!user || !isConnected) return blends;

    try {
        // Try the new /users/:userID/blends endpoint first (returns all blends).
        const std::string response = http.get(http.build_user_blends_endpoint(user->getUserID()));
        if (http.wasLastRequestSuccessful(http.G)) {
            Json parsed = Json::parse(response, nullptr, false);
            if (!parsed.is_discarded() && parsed.contains("blends") && parsed["blends"].is_array()) {
                for (const auto& item : parsed["blends"]) {
                    if (!item.is_object()) continue;
                    BlendSummary s;
                    s.blendID = item.value("blend_id", "");
                    s.title   = item.value("title", s.blendID);
                    if (!s.blendID.empty())
                        blends.push_back(std::move(s));
                }
                return blends;
            }
        }

        // Fallback: old server only has GET /users/:userID/blend (single latest blend).
        const std::string fallback = http.get(http.build_latest_blend_endpoint(user->getUserID()));
        if (http.wasLastRequestSuccessful(http.G)) {
            std::optional<std::string> blendID = parseBlendID(fallback);
            if (blendID.has_value() && !blendID->empty()) {
                BlendSummary s;
                s.blendID = *blendID;
                // Try to use the active blend's title if it matches
                Blend* active = appState.getActiveBlend();
                s.title = (active && active->getBlendID() == s.blendID)
                          ? active->getTitle() : s.blendID;
                if (s.title.empty()) s.title = s.blendID;
                blends.push_back(std::move(s));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[AppController] fetchUserBlends: " << e.what() << "\n";
    }
    return blends;
}

void AppController::handleLeaveBlend(const EventPayload& payload) {
    auto blendIt = payload.find("blendID");
    auto titleIt = payload.find("blendTitle");
    if (blendIt == payload.end()) return;

    User* user = appState.getCurrentUser();
    if (!user) return;

    const std::string& blendID = blendIt->second;
    const std::string blendTitle = (titleIt != payload.end()) ? titleIt->second : blendID;

    // Send system message to chat before leaving
    if (isConnected) {
        try {
            const std::string systemMsg = user->getUsername() + " has left " + blendTitle + ".";
            // Post via the WebSocket-compatible chat message endpoint
            ChatRoom* room = appState.getChatRoom(blendID);
            if (room) {
                room->addMessage("system", systemMsg);
            }
        } catch (...) {}
    }

    // Notify server
    if (isConnected) {
        try {
            const std::string body =
                "{\"user_id\":" + ModelJson::quote(user->getUserID()) + "}";
            http.post(http.build_leave_blend_endpoint(blendID), body);
        } catch (const std::exception& e) {
            std::cerr << "[AppController] handleLeaveBlend: " << e.what() << "\n";
        }
    }

    // Clear local state if this was the active blend
    Blend* active = appState.getActiveBlend();
    if (active && active->getBlendID() == blendID) {
        currentBlend.reset();
        appState.setActiveBlend(nullptr);
    }

    std::cout << "[AppController] Left blend '" << blendTitle << "'\n";
}

void AppController::handleSelectBlend(const EventPayload& payload) {
    auto blendIt = payload.find("blendID");
    if (blendIt == payload.end()) return;

    const std::string& blendID = blendIt->second;

    // Fetch participants for this blend
    const std::string participantResponse = http.get(http.build_blend_participant_endpoint(blendID));
    if (!http.wasLastRequestSuccessful(http.G)) {
        std::cerr << "[AppController] handleSelectBlend: failed to fetch participants\n";
        return;
    }

    const std::vector<std::string> participantIDs = parseParticipantIDs(participantResponse);
    std::list<User> participants = loadParticipantsWithWatchLater(participantIDs, nullptr);

    if (participants.empty()) {
        std::cerr << "[AppController] handleSelectBlend: no participants have data\n";
        return;
    }

    // Extract title from payload if available
    auto titleIt = payload.find("blendTitle");
    std::string title = (titleIt != payload.end()) ? titleIt->second : "";

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, title)
    );
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    std::cout << "[AppController] Switched to blend '" << blendID
              << "' with " << currentBlend->size() << " videos\n";
}