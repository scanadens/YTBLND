#include "EventRouter.hpp"
#include <iostream>

EventRouter::EventRouter() {}

void EventRouter::registerListener(const std::string& eventName, EventHandler handler) {
    listeners[eventName] = std::move(handler);
}

void EventRouter::dispatch(const std::string& eventName, const EventPayload& payload) {
    auto it = listeners.find(eventName);
    if (it == listeners.end()) {
        std::cerr << "[EventRouter] Unknown event: " << eventName << "\n";
        return;
    }
    it->second(payload);
}

void EventRouter::deregisterAll() {
    listeners.clear();
}
