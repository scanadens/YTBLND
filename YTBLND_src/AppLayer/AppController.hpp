#ifndef APPCONTROLLER_HPP
#define APPCONTROLLER_HPP

/**
 * \file AppController.hpp
 * \brief Central application coordinator.
 * \author Jasmine Kumar 
 *
 * AppController is the only class that communicates with both the UI layer
 * and the backend (DataManager, parsers, algorithms).  UI panels dispatch
 * events through EventRouter; AppController handles them by updating AppState
 * and calling the appropriate backend services.
 *
 * No panel or backend class should call another directly — everything routes
 * through AppController.
 */

#include <memory>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventRouter.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ServiceLayer/SqliteDataManager.hpp"

class YouTubeDataParser;  ///< Stub for future use.

/// Central application coordinator.
class AppController {
    private:
        AppState&    appState;     ///< Singleton session state.
        EventRouter  eventRouter;  ///< Messenger between UI and controller.
        std::unique_ptr<SqliteDataManager> dataManager;

        /// Owns the most recently generated Blend so AppState can hold a raw pointer.
        std::unique_ptr<Blend> currentBlend;

        /**
         * Registers all UI event handlers on the EventRouter.
         * Called once during construction.
         */
        void registerEvents();

    public:
        /**
         * Initialises AppState, EventRouter, and backend services.
         * \param dbPath Path to the SQLite database file.
         *               Defaults to \c "ytblnd.db" for normal runtime;
         *               pass \c ":memory:" in tests to avoid data leakage.
         */
        explicit AppController(const std::string& dbPath = "ytblnd.db");

        /**
         * Persists any unsaved changes, deregisters all EventRouter listeners,
         * and releases backend resources.
         */
        ~AppController();

        /// \return Reference to the EventRouter so panels can call dispatch().
        EventRouter& getEventRouter();

        /**
         * Creates a new account and logs the user in on success.
         * \param payload { "userID": "...", "username": "...", "email": "...", "password": "..." }
         */
        void handleRegister(const EventPayload& payload);

        /**
         * Validates credentials and sets AppState::currentUser on success.
         * \param payload { "userID": "...", "password": "..." }
         */
        void handleLogin(const EventPayload& payload);

        /**
         * Persists the current user, clears AppState, and navigates to LoginPanel.
         * \param payload {}
         */
        void handleLogout(const EventPayload& payload);

        /**
         * Imports the file at filePath, enriches metadata via YouTube API,
         * and persists the result to the database.
         * \param payload { "filePath": "...", "userID": "..." }
         */
        void handleUploadData(const EventPayload& payload);

        /**
         * Runs the blend algorithm over the listed participants, stores the
         * result, and creates a ChatRoom for the new blend.
         * \param payload { "userID_0": "...", "userID_1": "...", ... }
         */
        void handleCreateBlend(const EventPayload& payload);

        /**
         * Opens the selected video (currently a stub).
         * \param payload { "videoID": "..." }
         */
        void handlePlayVideo(const EventPayload& payload);

        /**
         * Re-fetches YouTube data for the current user and refreshes the active panel.
         * \param payload {}
         */
        void handleRefresh(const EventPayload& payload);

        /**
         * Opens the ChatPanel for the active blend's ChatRoom.
         * No-ops if there is no active blend or no room for it.
         * \param payload {}
         */
        void handleOpenChat(const EventPayload& payload);

        /**
         * Validates the sender is a blend participant, then adds the message.
         * \param payload { "userID": "...", "text": "..." }
         */
        void handleSendMessage(const EventPayload& payload);
};

#endif
