#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <string>
#include <vector>

#include "AppState.h"
#include "EventRouter.h"

// Forward declarations — these classes are stubs and will be fully
// implemented in later stages. AppController depends on their interfaces
// but does not need their full definitions in the header.
class DataManager;
class YouTubeDataParser;

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
        DataManager* dataManager; // Stub — handles storage and parsing coordination
        // IBlendAlgorithm* blendAlgorithm; // Stub — added when algorithm layer is implemented

        // Registers all UI event handlers on the EventRouter.
        // Called once during construction.
        void registerEvents();

    public:
        // Initialises AppState (singleton), EventRouter, and stub backend classes.
        // Registers all event listeners.
        AppController();

        // Persists any unsaved changes, deregisters all EventRouter listeners,
        // and releases backend resources.
        ~AppController();

        // Returns the EventRouter so panels can call dispatch() on it.
        EventRouter& getEventRouter();

        // ── Event Handlers ────────────────────────────────────────────────────
        // These are called by EventRouter when the matching event is dispatched.
        // Panels never call these directly.

        // Payload: { "email": "...", "password": "..." }
        // Validates credentials, loads the User, sets AppState.currentUser.
        // Triggers a data refresh if the user's YouTube data is stale.
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