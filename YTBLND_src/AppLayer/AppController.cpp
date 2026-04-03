#include "AppController.hpp"
#include "../ServerConfig.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ModelLayer/JsonUtils.hpp"
#include "../ServiceLayer/YouTubeMetadataFetcher.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <list>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <optional>

using Json = nlohmann::json;
using namespace std;

namespace {

std::string resolveBackendBaseUrl() {
    return kAppBackendBaseUrl;
}

/// Threshold for deciding whether to enrich HTML upload metadata synchronously.
/// Large watch-history HTML exports can contain many IDs and block the UI thread if
/// enriched during upload. Small HTML files are enriched immediately for faster display.
/// Videos beyond this count are persisted with minimal metadata and enriched later
/// when the blend is displayed.
constexpr std::size_t kHtmlUploadMetadataThreshold = 500;

/// Maximum number of videos to enrich on initial file upload.
/// Even for small files, we limit enrichment to this many IDs to keep upload fast.
/// Remaining IDs will be enriched at blend-display time if needed.
constexpr std::size_t kMaxUploadEnrichmentCount = 50;

/// Maximum number of blend-feed videos to enrich during a single UI action.
/// Keeping this bounded prevents create/select/refresh flows from hanging.
constexpr std::size_t kMaxBlendFeedEnrichmentCount = 50;

/// Number of worker threads used to enrich blend-feed metadata in parallel.
constexpr std::size_t kBlendMetadataWorkerThreads = 3;

/// Worker count for login-time participant watch-later loading.
constexpr std::size_t kLoginParticipantLoadWorkerThreads = 3;

/// Cap metadata enrichment during login to keep the sign-in path responsive.
constexpr std::size_t kMaxLoginMetadataEnrichmentCount = 50;

bool isMetadataIncomplete(const Video& v) {
    return v.getTitle().empty() ||
           v.getChannelID().empty() ||
           v.getChannelName().empty() ||
           v.getThumbnailURL().empty() ||
           v.getChannelLogoURL().empty();
}

bool hasCompleteMetadata(const Video& v) {
    return !v.getTitle().empty() &&
           !v.getChannelID().empty() &&
           !v.getChannelName().empty() &&
           !v.getThumbnailURL().empty() &&
           !v.getChannelLogoURL().empty();
}

std::string lowerExtension(const std::string& filePath) {
    const std::size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "";
    }

    std::string ext = filePath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

}

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

std::list<User> AppController::loadParticipantsWithWatchLater(const std::vector<std::string>& participantIDs,
                                                              std::vector<std::string>* missingData,
                                                              bool enrichParticipantLists) {
    std::list<User> participants;

    for (const auto& uid : participantIDs) {
        std::string watchLaterResponse;
        try {
            watchLaterResponse = http.get(http.build_watch_later_endpoint(uid));
        } catch (const std::exception& ex) {
            std::cerr << "[AppController] loadParticipantsWithWatchLater: failed to fetch watch-later for '"
                      << uid << "' - " << ex.what() << "\n";
            if (missingData != nullptr) {
                missingData->push_back(uid);
            }
            continue;
        }

        if (!http.wasLastRequestSuccessful(http.G)) {
            if (missingData != nullptr) {
                missingData->push_back(uid);
            }
            continue;
        }

        std::list<Video> watchLater = parseWatchLaterVideos(watchLaterResponse);
        if (watchLater.empty()) {
            Json debug = Json::parse(watchLaterResponse, nullptr, false);
            if (debug.is_discarded() || !debug.is_object() || !debug.contains("videos")) {
                std::cerr << "[AppController] loadParticipantsWithWatchLater: malformed watch-later payload for '"
                          << uid << "'\n";
            }
            if (missingData != nullptr) {
                missingData->push_back(uid);
            }
            continue;
        }

        // Optional: enrich participant pools. For blend-create/select paths this
        // is intentionally disabled to avoid expensive full-list metadata pulls.
        if (enrichParticipantLists) {
            enrichIfMissingMetadata(uid, watchLater);
        }

        // create a local representation of the user
        User user(uid, uid, "", "");
        YouTubeData yt;
        yt.setWatchLaterVideos(watchLater);
        user.setYouTubeData(yt);
        participants.push_back(user);
    }

    return participants;
}

std::list<User> AppController::loadParticipantsWithWatchLaterParallel(
    const std::vector<std::string>& participantIDs,
    std::vector<std::string>* missingData,
    std::size_t workerThreads) {
    std::list<User> participants;
    if (participantIDs.empty() || workerThreads == 0) {
        return participants;
    }

    const std::size_t workerCount = std::min<std::size_t>(workerThreads, participantIDs.size());
    std::vector<std::optional<User>> usersByIndex(participantIDs.size());
    std::mutex missingMutex;
    std::atomic<std::size_t> completed{0};
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (std::size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
        workers.emplace_back([&, workerIndex]() {
            HttpClient threadHttp(server_url);

            for (std::size_t i = workerIndex; i < participantIDs.size(); i += workerCount) {
                const std::string& uid = participantIDs[i];
                try {
                    const std::string response = threadHttp.get(threadHttp.build_watch_later_endpoint(uid));
                    if (!threadHttp.wasLastRequestSuccessful(threadHttp.G)) {
                        if (missingData != nullptr) {
                            std::lock_guard<std::mutex> lock(missingMutex);
                            missingData->push_back(uid);
                        }
                        completed.fetch_add(1, std::memory_order_relaxed);
                        continue;
                    }

                    std::list<Video> watchLater = parseWatchLaterVideos(response);
                    if (watchLater.empty()) {
                        if (missingData != nullptr) {
                            std::lock_guard<std::mutex> lock(missingMutex);
                            missingData->push_back(uid);
                        }
                        completed.fetch_add(1, std::memory_order_relaxed);
                        continue;
                    }

                    User user(uid, uid, "", "");
                    YouTubeData yt;
                    yt.setWatchLaterVideos(watchLater);
                    user.setYouTubeData(yt);
                    usersByIndex[i] = user;
                } catch (const std::exception&) {
                    if (missingData != nullptr) {
                        std::lock_guard<std::mutex> lock(missingMutex);
                        missingData->push_back(uid);
                    }
                }

                completed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    while (completed.load(std::memory_order_relaxed) < participantIDs.size()) {
        const double ratio = static_cast<double>(completed.load(std::memory_order_relaxed)) /
                             static_cast<double>(participantIDs.size());
        reportProgress(0.6 + (ratio * 0.22), "Loading blend participant data...");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    for (const auto& maybeUser : usersByIndex) {
        if (maybeUser.has_value()) {
            participants.push_back(*maybeUser);
        }
    }

    return participants;
}

static const std::string kYouTubeAPIKey = "AIzaSyBDzH4_T9NSaAh_s59sFYrHtNnKvHmyARM";

// ── Constructor / Destructor ──────────────────────────────────────────────────

AppController::AppController(): 
    appState(AppState::getInstance()),
    server_url(resolveBackendBaseUrl()), 
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
    // If application shutting down and there's a server-created pending account,
    // attempt to delete it so username isn't left reserved.
    if (pendingAccountCreated_ && pendingRegistration_.has_value() && isConnected) {
        try {
            const std::string delResp = http.del(
                http.build_delete_user_endpoint(pendingRegistration_->getUserID()),
                RequestJsonBuilder::buildDeleteAccountJson(pendingRegistration_->getPassword())
            );
            if (http.wasLastRequestSuccessful(http.D)) {
                std::cout << "[AppController] Deleted pending account '" << pendingRegistration_->getUserID()
                          << "' on shutdown\n";
            } else {
                std::cerr << "[AppController] Failed to delete pending account on shutdown, HTTP "
                          << http.getLastStatusCode() << " response=" << delResp << "\n";
            }
        } catch (...) {
            // best-effort; swallow errors
        }
    }

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

void AppController::setProgressReporter(std::function<void(double, const std::string&)> reporter) {
    progressReporter = std::move(reporter);
}

void AppController::clearProgressReporter() {
    progressReporter = nullptr;
}

void AppController::reportProgress(double progress, const std::string& message) {
    if (!progressReporter) {
        return;
    }

    if (progress < 0.0) progress = 0.0;
    if (progress > 1.0) progress = 1.0;
    progressReporter(progress, message);
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

    // Build a local User object
    User newUser(idIt->second,
                 unIt->second,
                 emIt != payload.end() ? emIt->second : "",
                 pwIt->second);

    // Attempt to create the account on the server immediately so we can delete it if
    // the user cancels or exits before uploading their data (backend requires it).
    if (isConnected) {
        try {
            const std::string registerResponse = http.post(
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
                    std::cerr << "[AppController] handleRegister: userID '"
                              << newUser.getUserID() << "' already exists\n";
                } else {
                    std::cerr << "[AppController] handleRegister: registration failed, HTTP "
                              << http.getLastStatusCode() << " response=" << registerResponse << "\n";
                }
                return;
            }

            // success — keep pendingRegistration_ so we can delete it if upload never happens
            pendingRegistration_ = newUser;
            pendingAccountCreated_ = true;
            std::cout << "[AppController] Account created on server for '" << newUser.getUserID()
                      << "' (pending upload)\n";
        } catch (const std::exception& ex) {
            std::cerr << "[AppController] handleRegister: HTTP error creating account: "
                      << ex.what() << "\n";
            return;
        }
    } else {
        // Offline: still store pendingRegistration_ locally (account not created on server).
        pendingRegistration_ = newUser;
        pendingAccountCreated_ = false;
        std::cout << "[AppController] Pending local registration stored for '" << newUser.getUserID()
                  << "' (offline)\n";
    }

    // Set the current user locally so the UI can continue to the DataInstructionsPanel.
    appState.setCurrentUser(new User(newUser));
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

    reportProgress(0.05, "Authenticating account...");

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

    reportProgress(0.2, "Loading your saved data...");

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
        reportProgress(0.3, "Refreshing metadata subset...");

        // Keep login responsive by enriching only a bounded subset and skipping
        // DB write-back on this path. Full enrichment can happen later during
        // upload/blend-display workflows.
        watchLater = enrichMissingMetadataSubsetMultithreaded(
            watchLater,
            kMaxLoginMetadataEnrichmentCount,
            kBlendMetadataWorkerThreads,
            [](const std::list<Video>& chunk) {
                return YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(chunk);
            }
        );
        reportProgress(0.42, "Metadata step complete");
    }

    if (!watchLater.empty()) {
        YouTubeData yd;
        yd.setWatchLaterVideos(watchLater);
        user->setYouTubeData(yd);
        appState.addSessionUser(*user);
    }

    appState.setCurrentUser(new User(*user));
    std::cout << "[AppController] Logged in as '" << user->getUsername() << "'\n";
    reportProgress(0.45, "Checking blend memberships...");

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
    reportProgress(0.6, "Loading blend participant data...");
    std::list<User> participants = loadParticipantsWithWatchLaterParallel(
        participantIDs,
        nullptr,
        kLoginParticipantLoadWorkerThreads
    );

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
    reportProgress(0.85, "Generating blend feed...");
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, restoredTitle)
    );
    currentBlend->setBlendID(*blendID);
    // Attempt to fill in missing metadata for the restored blend's feed videos.
    // If this blend contains videos from a large HTML import, they may lack title/channel
    // data until this point. Metadata enrichment here is deferred and happens on-demand.
    enrichActiveBlendFeedIfMissingMetadata();
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);

    // If this user has no data, leave a message for LoginPanel to display
    if (watchLater.empty()) {
        appState.setPendingBlendMessage(
            "You have been added to a blend. Upload your YouTube playlist "
            "data to contribute your videos to it.");
    }

    std::cout << "[AppController] Restored blend '" << *blendID
              << "' with " << currentBlend->size() << " videos\n";
    reportProgress(1.0, "Login complete");
}

void AppController::handleLogout(const EventPayload& payload) {
    // If there is a server-created pending account and the user is logging out
    // before uploading, attempt to delete it.
    if (pendingAccountCreated_ && pendingRegistration_.has_value() && isConnected) {
        try {
            const std::string delResp = http.del(
                http.build_delete_user_endpoint(pendingRegistration_->getUserID()),
                RequestJsonBuilder::buildDeleteAccountJson(pendingRegistration_->getPassword())
            );
            if (http.wasLastRequestSuccessful(http.D)) {
                std::cout << "[AppController] Deleted pending account '"
                          << pendingRegistration_->getUserID() << "' on logout\n";
            } else {
                std::cerr << "[AppController] Failed to delete pending account on logout, HTTP "
                          << http.getLastStatusCode() << " response=" << delResp << "\n";
            }
        } catch (const std::exception& ex) {
            std::cerr << "[AppController] handleLogout: failed to delete pending account: " << ex.what() << "\n";
        }
        pendingRegistration_.reset();
        pendingAccountCreated_ = false;
    }

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
        appState.setPendingUploadError("Upload failed: missing file path or user ID.");
        return;
    }

    const std::string& filePath = fileIt->second;
    const std::string& userID   = userIt->second;

    // If there's a pending registration for this user, create the server account first.
    if (pendingRegistration_.has_value() && pendingRegistration_->getUserID() == userID) {
        if (!isConnected) {
            appState.setPendingUploadError("Cannot create account while offline. Please connect and try again.");
            return;
        }
        std::string registerResponse;
        try {
            registerResponse = http.post(
                http.REG_USER,
                RequestJsonBuilder::buildRegisterJson(
                    pendingRegistration_->getUserID(),
                    pendingRegistration_->getUsername(),
                    pendingRegistration_->getEmail(),
                    pendingRegistration_->getPassword()
                )
            );
            if (!http.wasLastRequestSuccessful(http.P)) {
                if (http.getLastStatusCode() == http.DUPLICATE) {
                    appState.setPendingUploadError("Registration failed: username already taken. Choose a different username.");
                } else {
                    appState.setPendingUploadError("Registration failed: server returned an error. Please try again.");
                }
                // clear pending to allow retry or new username flow
                pendingRegistration_.reset();
                return;
            }
            // success — clear pendingRegistration_
            pendingRegistration_.reset();
            std::cout << "[AppController] Account created on server for '" << userID << "'\n";
        } catch (const std::exception& ex) {
            appState.setPendingUploadError("Registration failed: network error while creating account. Please try again.");
            std::cerr << "[AppController] handleUploadData: failed to create account for '" << userID << "' - " << ex.what() << "\n";
            return;
        }
    }

    std::cout << "[AppController] Starting file upload process for '" << filePath << "'\n";

    std::list<Video> watchLater;
    try {
        // Multi-threaded import with progress feedback via logging.
        // Divides file processing across 3 worker threads for ~3x faster parsing of large files.
        // Progress callback logs to stdout so users can monitor progress.
        auto progressCallback = [](double progress) {
            int percent = static_cast<int>(progress * 100);
            std::cout << "[AppController] File parsing progress: " << percent << "%\n";
        };

        std::cout << "[AppController] Starting multi-threaded file import...\n";
        watchLater = YouTubeDataImporter().importMultiThreaded(filePath, progressCallback);
        std::cout << "[AppController] File import complete: " << watchLater.size() 
                  << " videos extracted\n";
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] handleUploadData: import failed for '"
                  << filePath << "' - " << ex.what() << "\n";
        appState.setPendingUploadError(
            "Upload failed: could not parse this file. Please select a supported Google Takeout export (.csv, .html, or .htm)."
        );
        return;
    }

    if (watchLater.empty()) {
        std::cerr << "[AppController] handleUploadData: no videos found in '"
                  << filePath << "'\n";
        appState.setPendingUploadError(
            "No videos were found in the selected file. Please verify you selected a valid YouTube playlist CSV or watch-history HTML export from Google Takeout."
        );
        return;
    }

    const std::string fileExt = lowerExtension(filePath);
    const bool isCsvUpload = (fileExt == ".csv");
    const bool isHtmlUpload = (fileExt == ".html" || fileExt == ".htm");

    // Smart metadata enrichment strategy:
    // - On upload, enrich ONLY the first kMaxUploadEnrichmentCount videos to keep upload fast.
    // - CSV: always enrich first 50
    // - HTML ≤500 videos: enrich all
    // - HTML >500 videos: enrich first 50, rest at display time
    //
    // This avoids blocking the UI while still providing good metadata for the most important videos.

    std::list<Video> videosToEnrich;
    std::list<Video> videosNotEnriched;

    if (isHtmlUpload && watchLater.size() > kHtmlUploadMetadataThreshold) {
        // Large HTML: enrich only first 50
        auto it = watchLater.begin();
        std::advance(it, std::min(watchLater.size(), kMaxUploadEnrichmentCount));
        videosToEnrich.splice(videosToEnrich.end(), watchLater, watchLater.begin(), it);
        videosNotEnriched.splice(videosNotEnriched.end(), watchLater);
        std::cout << "[AppController] Large HTML upload (" << (videosToEnrich.size() + videosNotEnriched.size())
                  << " videos): enriching first " << videosToEnrich.size() << ", deferring "
                  << videosNotEnriched.size() << "\n";
    } else if (isCsvUpload && watchLater.size() > kMaxUploadEnrichmentCount) {
        // CSV with many videos: enrich first 50
        auto it = watchLater.begin();
        std::advance(it, std::min(watchLater.size(), kMaxUploadEnrichmentCount));
        videosToEnrich.splice(videosToEnrich.end(), watchLater, watchLater.begin(), it);
        videosNotEnriched.splice(videosNotEnriched.end(), watchLater);
        std::cout << "[AppController] CSV with " << (videosToEnrich.size() + videosNotEnriched.size())
                  << " videos: enriching first " << videosToEnrich.size() << "\n";
    } else {
        // Small HTML or CSV: enrich all
        videosToEnrich = watchLater;
        std::cout << "[AppController] Small file (" << videosToEnrich.size()
                  << " videos): enriching all\n";
    }

    // Execute metadata enrichment only on the manageable subset.
    bool enriched = false;
    if (!videosToEnrich.empty()) {
        std::cout << "[AppController] Starting metadata enrichment for "
                  << videosToEnrich.size() << " videos...\n";
        try {
            videosToEnrich = YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(videosToEnrich);
            enriched = !kYouTubeAPIKey.empty();
            std::cout << "[AppController] Metadata enrichment complete\n";
        } catch (const std::exception& ex) {
            std::cerr << "[AppController] handleUploadData: metadata fetch failed: "
                      << ex.what() << "\n";
            // Non-fatal — continue with whatever data we have
        }
    }

    // Recombine enriched and non-enriched videos.
    videosToEnrich.splice(videosToEnrich.end(), videosNotEnriched);
    watchLater = videosToEnrich;

    // Drop videos the API couldn't find — they are deleted, private, or unavailable.
    if (enriched) {
        watchLater.remove_if([](const Video& v) { return v.getTitle().empty(); });
        std::cout << "[AppController] " << watchLater.size()
                  << " videos available after filtering unavailable entries\n";
    }

    std::cout << "[AppController] Uploading " << watchLater.size() 
              << " videos to server...\n";

    // Attempt to post data to the server
    std::string saveWatchLaterResponse;
    try {
        saveWatchLaterResponse = 
            http.post(
                http.build_watch_later_endpoint(userID),
                RequestJsonBuilder::buildWatchLaterJson(watchLater)
            );
        if (!http.wasLastRequestSuccessful(http.P)) {
            std::cerr << "[AppController] handleUploadData: failed to persist watch-later list, HTTP "
                    << http.getLastStatusCode() << " response=" << saveWatchLaterResponse << "\n";
            appState.setPendingUploadError("Upload failed: server could not save your data. Please try again.");
            return;
        }

        cout << "[AppController] handleUploadData: Successful upload" << endl;
    } catch (exception& e) {
        std::cerr << "[AppController] handleUploadData: upload failed." << endl;
        cerr << "[AppController] handleUploadData - server response: " << saveWatchLaterResponse << endl;
        appState.setPendingUploadError("Upload failed: unable to reach the server. Please try again.");
        return;
    }

    // Clear pending registration state (account created + upload complete)
    if (pendingRegistration_.has_value() && pendingRegistration_->getUserID() == userID) {
        pendingRegistration_.reset();
        pendingAccountCreated_ = false;
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
    appState.setPendingUploadError("");

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
    std::list<User> participants = loadParticipantsWithWatchLater(participantIDs, nullptr, false);

    if (participants.empty()) {
        return;
    }

    // Preserve existing blend title if available
    std::string existingTitle;
    Blend* active = appState.getActiveBlend();
    if (active) existingTitle = active->getTitle();

    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, existingTitle)
    );
    currentBlend->setBlendID(*blendID);
    // After regenerating the blend with new participant data, try to enrich
    // any videos that may be missing metadata (e.g., from large HTML uploads).
    enrichActiveBlendFeedIfMissingMetadata();
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

    reportProgress(0.1, "Loading participant data...");
    std::vector<std::string> missingData;
    std::list<User> participants =
        loadParticipantsWithWatchLater(allParticipantIDs, &missingData, false);

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
    reportProgress(0.3, "Generating blend...");

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

    reportProgress(0.55, "Saving blend to server...");
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

    // Attempt to fill in missing metadata for the newly created blend's videos.
    // If participants have large HTML-imported watch lists, their contribution videos
    // may lack enriched metadata until now.
    reportProgress(0.8, "Enriching video metadata...");
    enrichActiveBlendFeedIfMissingMetadata();
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);
    appState.setIsBlendGenerating(false);
    reportProgress(1.0, "Blend created!");

    // Register with server so the chat room is provisioned there too.
    registerBlendOnServer(*currentBlend, creatorID);

    std::cout << "[AppController] Blend created with "
              << currentBlend->size() << " videos\n";
}

bool AppController::lookupUser(const std::string& userID) {
    try {
        // User existence is validated via auth profile lookup. This avoids
        // false negatives for valid users who have not uploaded watch-later yet.
        http.get(http.build_auth_user_lookup_endpoint(userID));
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

std::list<Video> AppController::enrichMissingMetadataSubsetMultithreaded(
    const std::list<Video>& videos,
    std::size_t maxVideosToEnrich,
    std::size_t workerThreads,
    const std::function<std::list<Video>(const std::list<Video>&)>& enrichChunkFn) {
    if (videos.empty() || maxVideosToEnrich == 0 || workerThreads == 0) {
        return videos;
    }

    std::list<Video> updatedVideos = videos;

    std::vector<Video> pending;
    pending.reserve(std::min(maxVideosToEnrich, videos.size()));
    for (const auto& video : videos) {
        if (!isMetadataIncomplete(video)) {
            continue;
        }
        pending.push_back(video);
        if (pending.size() >= maxVideosToEnrich) {
            break;
        }
    }

    if (pending.empty()) {
        return updatedVideos;
    }

    const std::size_t workerCount = std::max<std::size_t>(
        1,
        std::min<std::size_t>(workerThreads, pending.size())
    );

    std::unordered_map<std::string, Video> enrichedByID;
    std::mutex resultMutex;
    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (std::size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
        const std::size_t start = workerIndex * pending.size() / workerCount;
        const std::size_t end = (workerIndex + 1) * pending.size() / workerCount;
        if (start >= end) {
            continue;
        }

        workers.emplace_back([&, start, end]() {
            std::list<Video> chunk;
            for (std::size_t i = start; i < end; ++i) {
                chunk.push_back(pending[i]);
            }

            std::list<Video> chunkEnriched;
            try {
                chunkEnriched = enrichChunkFn(chunk);
            } catch (...) {
                return;
            }

            std::lock_guard<std::mutex> lock(resultMutex);
            for (const auto& enriched : chunkEnriched) {
                if (hasCompleteMetadata(enriched)) {
                    enrichedByID.insert_or_assign(enriched.getVideoID(), enriched);
                }
            }
        });
    }

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    for (auto& video : updatedVideos) {
        auto enrichedIt = enrichedByID.find(video.getVideoID());
        if (enrichedIt != enrichedByID.end()) {
            video = enrichedIt->second;
        }
    }

    return updatedVideos;
}

void AppController::enrichActiveBlendFeedIfMissingMetadata() {
    // Re-enriches only the currently displayed blend's video feed when any metadata
    // fields are incomplete. This keeps display quality high without re-processing
    // full watch-history sized lists on the UI path.
    //
    // This complements handleUploadData which may skip enrichment for large HTML
    // uploads. By deferring metadata fetch to blend-display time, we:
    // - Keep upload fast (avoid blocking the UI during file ingestion).
    // - Enrich only a small subset of videos (the active blend feed).
    // - Still provide rich metadata when users view their blend.

    if (!currentBlend) {
        return;
    }

    std::list<Video> feedVideos = currentBlend->getVideoList();
    const bool needsEnrichment =
        std::any_of(feedVideos.begin(), feedVideos.end(), isMetadataIncomplete);

    if (!needsEnrichment) {
        return;
    }

    std::size_t incompleteCount = 0;
    for (const auto& video : feedVideos) {
        if (isMetadataIncomplete(video)) {
            ++incompleteCount;
        }
    }
    const std::size_t deferredCount = incompleteCount > kMaxBlendFeedEnrichmentCount
                                        ? (incompleteCount - kMaxBlendFeedEnrichmentCount)
                                        : 0;

    std::cout << "[AppController] enrichActiveBlendFeedIfMissingMetadata: enriching up to "
              << kMaxBlendFeedEnrichmentCount << " blend videos ("
              << deferredCount << " deferred)\n";

    try {
        const std::list<Video> enrichedVideos = enrichMissingMetadataSubsetMultithreaded(
            feedVideos,
            kMaxBlendFeedEnrichmentCount,
            kBlendMetadataWorkerThreads,
            [](const std::list<Video>& chunk) {
                return YouTubeMetadataFetcher(kYouTubeAPIKey).enrich(chunk);
            }
        );

        std::size_t appliedCount = 0;
        auto oldIt = feedVideos.begin();
        auto newIt = enrichedVideos.begin();
        for (; oldIt != feedVideos.end() && newIt != enrichedVideos.end(); ++oldIt, ++newIt) {
            if (isMetadataIncomplete(*oldIt) && hasCompleteMetadata(*newIt)) {
                ++appliedCount;
            }
        }

        if (appliedCount > 0) {
            currentBlend->setVideoList(enrichedVideos);
        }

        std::cout << "[AppController] enrichActiveBlendFeedIfMissingMetadata: completed "
                  << appliedCount << " updates using "
                  << kBlendMetadataWorkerThreads << " workers\n";
    } catch (const std::exception& ex) {
        std::cerr << "[AppController] enrichActiveBlendFeedIfMissingMetadata failed: "
                  << ex.what() << "\n";
    }
}

void AppController::handlePlayVideo(const EventPayload& payload) {
    // TODO: read "videoID" from payload
    // TODO: open the video — exact mechanism (browser, in-app) TBD
    std::cout << "[AppController] handlePlayVideo called (stub)\n";
}

void AppController::handleRefresh(const EventPayload& /*payload*/) {
    reportProgress(0.05, "Preparing blend refresh...");

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
    reportProgress(0.2, "Loading participant video pools...");

    // Lightweight load for refresh: fetch each participant's watch-later list
    // but DO NOT call enrichIfMissingMetadata (that can trigger slow network calls).
    std::list<User> participants;
    for (std::size_t i = 0; i < participantIDs.size(); ++i) {
        const auto& uid = participantIDs[i];
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

        if (!participantIDs.empty()) {
            const double ratio = static_cast<double>(i + 1) / static_cast<double>(participantIDs.size());
            reportProgress(0.2 + (ratio * 0.35), "Loading participant video pools...");
        }
    }

    if (participants.empty()) {
        std::cerr << "[AppController] handleRefresh: no participants have data\n";
        return;
    }

    std::string refreshTitle = existing->getTitle();
    std::string refreshID = existing->getBlendID();
    reportProgress(0.62, "Generating refreshed blend...");
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, refreshTitle)
    );
    currentBlend->setBlendID(refreshID);

    // Prevent duplicate videos within the same refreshed blend payload.
    std::set<std::string> seenVideoIDs;
    std::list<Video> uniqueVideos;
    for (const auto& video : currentBlend->getVideoList()) {
        if (seenVideoIDs.insert(video.getVideoID()).second) {
            uniqueVideos.push_back(video);
        }
    }
    currentBlend->setVideoList(uniqueVideos);
    reportProgress(0.8, "Enriching missing metadata...");
    // Attempt to enrich any missing metadata in the refreshed blend feed.
    // After filtering and re-sampling, some videos may lack enriched fields, especially
    // if the participant data originated from large HTML imports.
    enrichActiveBlendFeedIfMissingMetadata();

    appState.setActiveBlend(currentBlend.get());
    reportProgress(1.0, "Refresh complete");

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
    std::list<Message> result;

    Json parsed = Json::parse(response, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("messages")) {
        return result;
    }

    const Json& messages = parsed["messages"];
    if (!messages.is_array()) {
        return result;
    }

    for (const auto& item : messages) {
        if (!item.is_object()) {
            continue;
        }

        const std::string sender  = item.value("sender_id", "");
        const std::string content = item.value("content", "");
        const std::string sentAt  = item.value("sent_at", "");

        if (sender.empty() || content.empty()) {
            continue;
        }

        std::time_t ts = std::time(nullptr);
        if (!sentAt.empty()) {
            std::tm tm{};
            if (strptime(sentAt.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm) != nullptr) {
                ts = timegm(&tm);
            }
        }

        result.emplace_back(sender, content, ts);
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
    reportProgress(0.1, "Fetching participants...");
    const std::string participantResponse = http.get(http.build_blend_participant_endpoint(blendID));
    if (!http.wasLastRequestSuccessful(http.G)) {
        std::cerr << "[AppController] handleSelectBlend: failed to fetch participants\n";
        return;
    }

    reportProgress(0.3, "Loading participant data...");
    const std::vector<std::string> participantIDs = parseParticipantIDs(participantResponse);
    std::list<User> participants = loadParticipantsWithWatchLater(participantIDs, nullptr, false);

    if (participants.empty()) {
        std::cerr << "[AppController] handleSelectBlend: no participants have data\n";
        return;
    }

    // Extract title from payload if available
    auto titleIt = payload.find("blendTitle");
    std::string title = (titleIt != payload.end()) ? titleIt->second : "";

    reportProgress(0.6, "Generating blend feed...");
    currentBlend = std::make_unique<Blend>(
        RandomBlendAlgorithm(5).generateBlend(participants, title)
    );
    currentBlend->setBlendID(blendID);
    // Attempt to fill in missing metadata for the selected blend's feed.
    // If the blend contains videos from an HTML import that was saved without enrichment,
    // metadata will be fetched here to provide a complete display experience.
    reportProgress(0.8, "Enriching video metadata...");
    enrichActiveBlendFeedIfMissingMetadata();
    appState.setActiveBlend(currentBlend.get());
    appState.createChatRoom(*currentBlend);
    reportProgress(1.0, "Blend loaded!");

    std::cout << "[AppController] Switched to blend '" << blendID
              << "' with " << currentBlend->size() << " videos\n";
}