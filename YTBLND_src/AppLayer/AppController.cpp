#include "AppController.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ServiceLayer/YouTubeMetadataFetcher.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"
#include <algorithm>
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

    // Report users without uploaded data back to the UI via AppState
    // (non-existent users are excluded — they should have been rejected at the UI level)
    appState.setUsersWithoutData(missingData);

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

    std::list<User> loadedParticipants = loadParticipantsWithWatchLater(participantIDs, nullptr);
    std::list<User> participants;
    for (User p : loadedParticipants) {
        std::list<Video> allVideos = p.getYouTubeData().getWatchLaterVideos();
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
        p.setYouTubeData(pyd);
        participants.push_back(p);
    }

    if (participants.empty()) {
        std::cerr << "[AppController] handleRefresh: no participants have data\n";
        return;
    }

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants)
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

std::shared_ptr<User> AppController::get_current_user() {
    std::shared_ptr<User> u (appState.getCurrentUser());
    
    return u;
}

std::string AppController::get_current_username() {
    if (appState.getCurrentUser() == nullptr) return "";

    return appState.getCurrentUser()->getUsername();
}

std::string AppController::get_current_email() {
    if (appState.getCurrentUser() == nullptr) return "";

    return appState.getCurrentUser()->getEmail();
}