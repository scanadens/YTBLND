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
#include <list>
#include <optional>
#include <string>
#include <vector>

#include "AppState.hpp"
#include "EventRouter.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ServiceLayer/HttpClient.hpp"

class YouTubeDataParser;  ///< Stub for future use.

/// Central application coordinator.
class AppController {
    private:
        AppState&    appState;    // Singleton session state
        EventRouter  eventRouter; // Messenger between UI and controller
        const std::string server_url;       // url and port to the server
        HttpClient http;                    // http client wrapper for server communications
        bool isConnected;                   // tracks whether AppState has a connection to the server

        /// Owns the most recently generated Blend so AppState can hold a raw pointer.
        std::unique_ptr<Blend> currentBlend;

        /**
         * Registers all UI event handlers on the EventRouter.
         * Called once during construction.
         */
        void registerEvents();

        /**
         * Re-enriches \p videos via the YouTube API if any are missing a title.
         * Saves the updated list back to the database when enrichment occurs.
         * No-op if all videos already have titles.
         * \param userID Owner of the video list (used for the DB save).
         * \param videos Video list to inspect and potentially update in-place.
         */
        void enrichIfMissingMetadata(const std::string& userID,
                                     std::list<Video>&  videos);
        /**
         * \brief Parses an authenticated user object from the login response payload.
         *
         * Expects a JSON object that includes at minimum a non-empty \c user_id field.
         * Optional fields such as \c username and \c email are used when present.
         *
         * \param response Raw JSON response body returned by the auth endpoint.
         * \param fallbackPassword Password provided at login time, used to populate
         *        the in-memory \c User object when the response does not include one.
         * \return Parsed \c User on success, or \c std::nullopt when parsing fails
         *         or required fields are missing.
         */
        static std::optional<User> parseUserFromAuthResponse(const std::string& response,
                                     const std::string& fallbackPassword);

        /**
         * \brief Extracts a blend identifier from a blend-related JSON response.
         *
         * \param response Raw JSON response body expected to include \c blend_id.
         * \return Blend ID when present and non-empty; otherwise \c std::nullopt.
         */
        static std::optional<std::string> parseBlendID(const std::string& response);

        /**
         * \brief Parses participant user IDs from a blend participant response.
         *
         * \param response Raw JSON response body expected to include an array field
         *        named \c participants.
         * \return Vector of parsed participant IDs. Returns an empty vector when
         *         payload is malformed or the field is absent.
         */
        static std::vector<std::string> parseParticipantIDs(const std::string& response);

        /**
         * \brief Parses watch-later videos from a watch-later endpoint response.
         *
         * \param response Raw JSON response body expected to include an array field
         *        named \c videos.
         * \return List of parsed \c Video objects. Entries missing required \c video_id
         *         are skipped.
         */
        static std::list<Video> parseWatchLaterVideos(const std::string& response);

        /**
         * \brief Loads participant users and enriches each with watch-later data.
         *
         * For each user ID, this method fetches watch-later videos from the server and
         * constructs an in-memory \c User object containing the parsed \c YouTubeData.
         *
         * \param participantIDs Ordered list of user IDs to resolve.
         * \param missingData Optional output vector populated with user IDs that could
         *        not be loaded or had no available watch-later data.
         * \return List of participant users that have usable watch-later data.
         */
        std::list<User> loadParticipantsWithWatchLater(const std::vector<std::string>& participantIDs,
                                   std::vector<std::string>* missingData);

    public:
        /**
         * Initialises AppState, EventRouter, and backend services.
         */
        explicit AppController();

        /**
         * Compatibility constructor used by tests that inject a DB path.
         * The current HTTP-backed controller ignores this value.
         */
        explicit AppController(const std::string& dbPath);

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
         * Generates a new blend from the existing participants, excluding videos
         * already in the current blend.  Re-enriches any videos missing metadata
         * via the YouTube API before sampling.
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
         * Checks whether a user with the given ID exists in the database.
         * \param userID The user ID to look up.
         * \return \c true if the user was found, \c false otherwise.
         */
        bool lookupUser(const std::string& userID);

        /**
         * Validates the sender is a blend participant, then adds the message.
         * \param payload { "userID": "...", "text": "..." }
         */
        void handleSendMessage(const EventPayload& payload);
};

#endif
