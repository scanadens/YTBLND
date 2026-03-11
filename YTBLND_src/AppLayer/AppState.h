#ifndef APPSTATE_H
#define APPSTATE_H

#include <map>
#include <string>

#include "../ModelLayer/User.h"
#include "../ModelLayer/Blend.h"
#include "../ModelLayer/ChatRoom.h"

// AppState is the application's session-scoped memory.
// It holds what is currently happening while the program is running:
// who is logged in, what blend is active, and what chat rooms exist.
//
// This class is a Singleton — only one instance can ever exist.
// Use AppState::getInstance() to access it everywhere.
// The constructor is private to enforce this.
//
// AppState holds no business logic and performs no I/O.
// It is updated exclusively by AppController.
class AppState {
    private:
        User*  currentUser;       // Logged-in user. Null if no session is active.
        Blend* activeBlend;       // Most recently generated or loaded Blend. Null if none.
        bool   isBlendGenerating; // True while BlendAlgorithm is running.

        // All chat rooms for this session, keyed by blendID.
        // Currently only the active blend's room is used, but the map
        // structure allows multiple rooms to be supported in the future
        // without changing this class.
        std::map<std::string, ChatRoom> chatRooms;

        // Private constructor — use getInstance()
        AppState();

        // Delete copy constructor and assignment to prevent cloning the singleton
        AppState(const AppState&)            = delete;
        AppState& operator=(const AppState&) = delete;

    public:
        // Returns the single global AppState instance.
        // Creates it on first call; returns the same instance on every subsequent call.
        static AppState& getInstance();

        // ── Current User ──────────────────────────────────────────────────────
        User* getCurrentUser()               const;
        void  setCurrentUser(User* user);

        // ── Active Blend ──────────────────────────────────────────────────────
        Blend* getActiveBlend()              const;
        void   setActiveBlend(Blend* blend);

        // ── Blend Generation Flag ─────────────────────────────────────────────
        bool isGeneratingBlend()             const;
        void setIsBlendGenerating(bool flag);

        // ── Chat Rooms ────────────────────────────────────────────────────────

        // Returns a pointer to the ChatRoom for the active Blend.
        // Returns nullptr if there is no active blend or no room exists for it.
        ChatRoom* getActiveChatRoom();

        // Creates a ChatRoom for the given Blend using its participant IDs.
        // If a room already exists for that blendID it is replaced.
        void createChatRoom(const Blend& blend);

        // Returns a pointer to the ChatRoom for the given blendID.
        // Returns nullptr if no room exists for that ID.
        // Used to support multiple rooms in the future.
        ChatRoom* getChatRoom(const std::string& blendID);

        // ── Session ───────────────────────────────────────────────────────────

        // Resets all state: clears currentUser, activeBlend, isBlendGenerating,
        // and all chat rooms. Called by AppController on logout.
        void clearSession();
};

#endif