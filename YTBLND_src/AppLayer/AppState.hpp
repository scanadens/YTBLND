#ifndef APPSTATE_H
#define APPSTATE_H

/**
 * \file AppState.hpp
 * \brief Session-scoped singleton holding all runtime application state.
 *  \author Jasmine Kumar
 * AppState is the application's in-memory session store.  It records who is
 * logged in, what blend is active, and what chat rooms exist.
 *
 * This is a Singleton — use AppState::getInstance() everywhere.  The
 * constructor is private to enforce this.  AppState holds no business logic
 * and performs no I/O; it is updated exclusively by AppController.
 */

#include <map>
#include <string>
#include <vector>

#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ServiceLayer/HttpClient.hpp"

/// Session-scoped singleton holding all runtime application state.
class AppState {
    private:
        User*  currentUser;        ///< Logged-in user; null when no session is active.
        Blend* activeBlend;        ///< Most recently generated or loaded Blend; null if none.
        bool   isBlendGenerating;  ///< True while an IBlendAlgorithm is running.

        /// All chat rooms for this session, keyed by blendID.
        std::map<std::string, ChatRoom> chatRooms;

        /// Participants that have uploaded Watch Later data, keyed by userID.
        std::map<std::string, User> sessionUsers;

        /// Participants who had no data when the last blend was created.
        std::vector<std::string> usersWithoutData;

        /**
         * One-time message set by handleLogin when the user belongs to a blend
         * but has not yet uploaded data.  Cleared after being read.
         */
        std::string pendingBlendMessage;

        /**
         * One-time upload failure message set by handleUploadData when parsing,
         * enrichment, or persistence fails. Cleared after being read by UI.
         */
        std::string pendingUploadError;

        AppState();
        AppState(const AppState&)            = delete;
        AppState& operator=(const AppState&) = delete;

    public:
        /**
         * Returns the single global AppState instance.
         * Creates it on first call; returns the same instance on every subsequent call.
         */
        static AppState& getInstance();

        /// \return Pointer to the logged-in user, or nullptr if no session is active.
        User* getCurrentUser()               const;
        /// \param user Pointer to the newly logged-in User (AppController owns it).
        void  setCurrentUser(User* user);

        /// \return Pointer to the active Blend, or nullptr if none exists.
        Blend* getActiveBlend()              const;
        /// \param blend Raw pointer to the active Blend (AppController owns it).
        void   setActiveBlend(Blend* blend);

        /// \return True while an IBlendAlgorithm is running.
        bool isGeneratingBlend()             const;
        /// \param flag True to mark blend generation as in-progress.
        void setIsBlendGenerating(bool flag);

        /**
         * Returns a pointer to the ChatRoom for the active Blend.
         * \return Pointer to the room, or nullptr if no active blend or no room exists.
         */
        ChatRoom* getActiveChatRoom();

        /**
         * Creates a ChatRoom for the given Blend.
         * Replaces any existing room with the same blendID.
         * \param blend The Blend whose participants populate the new room.
         */
        void createChatRoom(const Blend& blend);

        /**
         * Returns a pointer to the ChatRoom for the given blendID.
         * \param blendID ID of the desired blend.
         * \return Pointer to the room, or nullptr if none exists.
         */
        ChatRoom* getChatRoom(const std::string& blendID);

        /**
         * Adds or replaces the session entry for this user.
         * \param user User who has uploaded blend data.
         */
        void addSessionUser(const User& user);

        /// \return All users that have uploaded data in this session.
        std::map<std::string, User> getSessionUsers() const;

        /// Removes all session participants (call before starting a new blend).
        void clearSessionUsers();

        /**
         * Stores userIDs of participants who had no Watch Later data when the
         * blend was last created.
         * \param users List of userIDs without data.
         */
        void setUsersWithoutData(const std::vector<std::string>& users);

        /// \return userIDs of participants without data from the last blend creation.
        std::vector<std::string> getUsersWithoutData() const;

        /**
         * Sets a one-time message shown by LoginPanel when the user is a blend
         * participant but has not yet uploaded data.
         * \param msg Message to display.
         */
        void setPendingBlendMessage(const std::string& msg);

        /**
         * Returns and clears the pending blend message.
         * \return The pending message string, then clears it.
         */
        std::string takePendingBlendMessage();

        /**
         * Sets a one-time message shown by DataInstructionsPanel when upload
         * failed or produced no usable videos.
         * \param msg Message to display.
         */
        void setPendingUploadError(const std::string& msg);

        /**
         * Returns and clears the pending upload error message.
         * \return The pending upload error string, then clears it.
         */
        std::string takePendingUploadError();

        /**
         * Resets all session state: clears currentUser, activeBlend,
         * isBlendGenerating, chat rooms, and session users.
         * Called by AppController on logout.
         */
        void clearSession();
};

#endif
