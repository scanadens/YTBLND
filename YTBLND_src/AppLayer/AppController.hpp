#ifndef APPCONTROLLER_HPP
#define APPCONTROLLER_HPP

#include <memory>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventRouter.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ServiceLayer/SqliteDataManager.hpp"
#include "../ServiceLayer/HttpClient.hpp"

class YouTubeDataParser;  // stub for future use

// AppController is the application's central coordinator.
// It is the only class that talks to both the UI layer and the backend.
//
// UI panels dispatch events through EventRouter.
// AppController handles those events by updating AppState and
// calling the appropriate backend classes (DataManager, parser, etc).
//
// No panel or backend class should call another directly —
// everything routes through AppController.
class AppController {
    private:
        AppState&    appState;    // Singleton session state
        EventRouter  eventRouter; // Messenger between UI and controller
        std::unique_ptr<SqliteDataManager> dataManager;
        HttpClient http;                    // http client wrapper for server communications
        const std::string server_url;       // url and port to the server
        bool isConnected;                   // tracks whether AppState has a connection to the server

        // IBlendAlgorithm* blendAlgorithm; // Stub — added when algorithm layer is implemented

        // Owns the most recently generated Blend so AppState can hold a raw pointer to it.
        std::unique_ptr<Blend> currentBlend;

        // Registers all UI event handlers on the EventRouter.
        // Called once during construction.
        void registerEvents();

    public:
        // Initialises AppState (singleton), EventRouter, and stub backend classes.
        // Registers all event listeners.
        // dbPath defaults to ytblnd.db for normal app runtime, but tests can
        // pass an isolated file path to avoid cross-test data leakage.
        explicit AppController(const std::string& dbPath = "ytblnd.db");

        // Persists any unsaved changes, deregisters all EventRouter listeners,
        // and releases backend resources.
        ~AppController();

        // Returns the EventRouter so panels can call dispatch() on it.
        EventRouter& getEventRouter();

        // ── Event Handlers ────────────────────────────────────────────────────
        // These are called by EventRouter when the matching event is dispatched.
        // Panels never call these directly.

        // Payload: { "userID": "...", "username": "...", "email": "...", "password": "..." }
        // Creates a new account and logs the user in on success.
        void handleRegister(const EventPayload& payload);

        // Payload: { "userID": "...", "password": "..." }
        // Validates credentials, loads the User, sets AppState.currentUser.
        void handleLogin(const EventPayload& payload);

        // Payload: {}
        // Persists the current user, clears AppState, navigates to LoginPanel.
        void handleLogout(const EventPayload& payload);

        // Payload: { "filePath": "..." }
        // Passes the file at filePath to YouTubeDataParser.
        // Updates the current user's YoutubeData in AppState on success.
        void handleUploadData(const EventPayload& payload);

        // Payload: { "userID_0": "...", "userID_1": "...", ... }
        // Collects the listed users, runs BlendAlgorithm, sets AppState.activeBlend,
        // creates a ChatRoom for the new blend, and persists the result.
        void handleCreateBlend(const EventPayload& payload);

        // Payload: { "videoID": "..." }
        // Opens the selected video. Exact behaviour (external browser, in-app player)
        // to be determined when the UI layer is implemented.
        void handlePlayVideo(const EventPayload& payload);

        // Payload: {}
        // Re-fetches or re-parses YouTube data for the current user and
        // re-renders the active panel.
        void handleRefresh(const EventPayload& payload);

        // Payload: {}
        // Opens the ChatPanel for the active blend's ChatRoom.
        // No-ops if there is no active blend or no chat room for it.
        void handleOpenChat(const EventPayload& payload);

        // Payload: { "userID": "...", "text": "..." }
        // Validates the sender is a participant in the active ChatRoom,
        // then adds the message to the room via AppState.
        void handleSendMessage(const EventPayload& payload);
};

#endif