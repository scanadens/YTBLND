#ifndef EVENTROUTER_H
#define EVENTROUTER_H

/**
 * \file EventRouter.hpp
 * \brief Lightweight event-dispatch system connecting the UI layer to AppController.
 *  \author Jasmine Kumar
 *
 * UI panels dispatch named events with a string-to-string payload map.
 * AppController registers a handler for each event name during construction.
 * Panels never call AppController directly; AppController never polls the UI.
 *
 * Payload conventions by event name:
 * - \c "login"       → { "userID": "...", "password": "..." }
 * - \c "logout"      → {}
 * - \c "deleteAccount" → { "userID": "...", "password": "..." }
 * - \c "uploadData"  → { "filePath": "...", "userID": "..." }
 * - \c "createBlend" → { "userID_0": "...", "userID_1": "..." }
 * - \c "playVideo"   → { "videoID": "..." }
 * - \c "refresh"     → {}
 * - \c "openChat"    → {}
 * - \c "sendMessage" → { "userID": "...", "text": "..." }
 */

#include <functional>
#include <map>
#include <string>

/// String-to-string map passed with every dispatched event.
using EventPayload = std::map<std::string, std::string>;

/// Any callable that accepts an EventPayload.
using EventHandler = std::function<void(const EventPayload&)>;

/// Lightweight event-dispatch system connecting the UI layer to AppController.
class EventRouter {
    private:
        std::map<std::string, EventHandler> listeners;

    public:
        EventRouter();

        /**
         * Registers a handler for the given event name.
         * Replaces any existing handler registered under the same name.
         * \param eventName Unique name for this event.
         * \param handler   Callable invoked when the event is dispatched.
         */
        void registerListener(const std::string& eventName, EventHandler handler);

        /**
         * Dispatches a named event to its registered handler.
         * Logs a warning and does nothing if the event name is unrecognised.
         * \param eventName Name of the event to dispatch.
         * \param payload   Key-value data passed to the handler.
         */
        void dispatch(const std::string& eventName, const EventPayload& payload = {});

        /**
         * Removes all registered listeners.
         * Called by AppController's destructor during teardown.
         */
        void deregisterAll();
};

#endif
