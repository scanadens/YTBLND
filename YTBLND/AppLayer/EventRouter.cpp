#ifndef EVENTROUTER_H
#define EVENTROUTER_H

#include <functional>
#include <map>
#include <string>

// Payload type — a simple string-to-string map.
// Examples of what each event puts in the payload:
//   "login"       → { "email": "...", "password": "..." }
//   "logout"      → {}
//   "uploadData"  → { "filePath": "..." }
//   "createBlend" → { "userID_0": "...", "userID_1": "..." }
//   "playVideo"   → { "videoID": "..." }
//   "refresh"     → {}
//   "openChat"    → {}
//   "sendMessage" → { "userID": "...", "text": "..." }
using EventPayload = std::map<std::string, std::string>;

// Handler type — any callable that accepts an EventPayload
using EventHandler = std::function<void(const EventPayload&)>;

// EventRouter is the application's messenger system.
// UI panels dispatch named events with a payload.
// EventRouter maps each event name to a handler registered by AppController.
//
// Panels never call AppController directly — they call router.dispatch().
// AppController never polls the UI — it only reacts to dispatched events.
class EventRouter {
    private:
        std::map<std::string, EventHandler> listeners;

    public:
        EventRouter();

        // Registers a handler for the given event name.
        // If a handler already exists for that name it is replaced.
        // Called by AppController's constructor to wire up all events.
        void registerListener(const std::string& eventName, EventHandler handler);

        // Called by UI panels when an event occurs.
        // Looks up the handler by eventName and calls it with the payload.
        // Logs a warning and does nothing if the event name is unrecognised.
        void dispatch(const std::string& eventName, const EventPayload& payload = {});

        // Removes all registered listeners.
        // Called by AppController's destructor during teardown.
        void deregisterAll();
};

#endif