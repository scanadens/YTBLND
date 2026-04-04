/**
 * \file Test_ChatPersistence.cpp
 * \brief Unit and integration tests for chat history persistence.
 * \author Jasmine Kumar
 *
 * \details
 * Validates chat persistence behavior for:
 * - AppController::parseChatHistory parsing of valid, empty, and malformed JSON.
 * - Message timestamp handling and fallback behavior for missing sent_at values.
 * - ChatRoom::addMessage(const Message&) timestamp preservation semantics.
 * - End-to-end live-backend flow: send message over WebSocket, then read via chat-history API.
 */

// Unit tests:
//   - parseChatHistory: valid / empty / malformed JSON
//   - ChatRoom::addMessage(const Message&): timestamp preserved
//
// Integration tests (run by default; opt-out via YTBLND_SKIP_LIVE_BACKEND_TESTS=1):
//   - Send a message over WebSocket, then retrieve it via /chat-history

#include "gtest/gtest.h"

#include "../ServerConfig.hpp"
#include "../AppLayer/AppController.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ModelLayer/JsonUtils.hpp"
#include "../ModelLayer/Message.hpp"
#include "../ServiceLayer/HttpClient.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// Helpers

namespace {

std::string makeUniqueID(const std::string& prefix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    const auto ns  = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    std::ostringstream out;
    out << prefix << "_" << ns;
    return out.str();
}

std::string getenvOrDefault(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return (val && std::string(val) != "") ? std::string(val) : fallback;
}

void skipUnlessLiveBackendEnabled() {
    // Explicit opt-out always wins.
    const char* flag = std::getenv("YTBLND_SKIP_LIVE_BACKEND_TESTS");
    if (flag && std::string(flag) == "1") {
        GTEST_SKIP() << "Live backend tests disabled by YTBLND_SKIP_LIVE_BACKEND_TESTS=1.";
    }

    // Skip gracefully if the configured server is not reachable.
    HttpClient probe(kTestBackendBaseUrl);
    try {
        const std::string resp = probe.get("/ping");
        if (!probe.wasLastRequestSuccessful(probe.G) ||
            resp.find("pong") == std::string::npos) {
            GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
        }
    } catch (...) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }
}

}  // namespace

// parseChatHistory unit tests

TEST(ParseChatHistoryTest, ParsesWellFormedHistoryCorrectly) {
    const std::string json =
        "{\"messages\":["
        "{\"sender_id\":\"alice\",\"content\":\"hello\",\"sent_at\":\"2026-03-26T15:04:05Z\"},"
        "{\"sender_id\":\"bob\",\"content\":\"world\",\"sent_at\":\"2026-03-26T15:04:10Z\"}"
        "]}";

    std::list<Message> msgs = AppController::parseChatHistory(json);

    ASSERT_EQ(msgs.size(), 2u);

    auto it = msgs.begin();
    EXPECT_EQ(it->userID, "alice");
    EXPECT_EQ(it->text,   "hello");
    // Verify timestamp was parsed from ISO 8601 (not defaulted to now)
    // 2026-03-26T15:04:05Z == 1774537445 (UTC)
    EXPECT_EQ(it->timestamp, static_cast<std::time_t>(1774537445));

    ++it;
    EXPECT_EQ(it->userID, "bob");
    EXPECT_EQ(it->text,   "world");
    EXPECT_EQ(it->timestamp, static_cast<std::time_t>(1774537450));
}

TEST(ParseChatHistoryTest, ReturnsEmptyListForEmptyMessagesArray) {
    const std::string json = "{\"messages\":[]}";
    std::list<Message> msgs = AppController::parseChatHistory(json);
    EXPECT_TRUE(msgs.empty());
}

TEST(ParseChatHistoryTest, ReturnsEmptyListForMissingMessagesKey) {
    const std::string json = "{\"other_field\":\"value\"}";
    std::list<Message> msgs = AppController::parseChatHistory(json);
    EXPECT_TRUE(msgs.empty());
}

TEST(ParseChatHistoryTest, ReturnsEmptyListForEmptyString) {
    std::list<Message> msgs = AppController::parseChatHistory("");
    EXPECT_TRUE(msgs.empty());
}

TEST(ParseChatHistoryTest, SkipsObjectsMissingSenderOrContent) {
    // Objects missing sender_id or content must not produce a Message.
    const std::string json =
        "{\"messages\":["
        "{\"sender_id\":\"alice\",\"sent_at\":\"2026-03-26T15:04:05Z\"},"
        "{\"content\":\"orphan\",\"sent_at\":\"2026-03-26T15:04:05Z\"},"
        "{\"sender_id\":\"bob\",\"content\":\"valid\",\"sent_at\":\"2026-03-26T15:04:05Z\"}"
        "]}";

    std::list<Message> msgs = AppController::parseChatHistory(json);
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs.front().userID, "bob");
    EXPECT_EQ(msgs.front().text,   "valid");
}

TEST(ParseChatHistoryTest, FallsBackToCurrentTimeWhenSentAtAbsent) {
    const std::string json =
        "{\"messages\":["
        "{\"sender_id\":\"alice\",\"content\":\"no timestamp\"}"
        "]}";

    const std::time_t before = std::time(nullptr);
    std::list<Message> msgs = AppController::parseChatHistory(json);
    const std::time_t after  = std::time(nullptr);

    ASSERT_EQ(msgs.size(), 1u);
    // Timestamp should be a recent wall-clock value, not an ancient epoch.
    EXPECT_GE(msgs.front().timestamp, before);
    EXPECT_LE(msgs.front().timestamp, after);
}

// ChatRoom::addMessage(const Message&) unit tests

TEST(ChatRoomAddMessageOverloadTest, PreservesOriginalTimestamp) {
    ChatRoom room("blend-1", {"user-1"});

    const std::time_t fixedTs = 1000000000; // 2001-09-09T01:46:40Z
    Message msg("user-1", "persisted text", fixedTs);
    room.addMessage(msg);

    const std::list<Message> msgs = room.getMessages();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs.front().userID,    "user-1");
    EXPECT_EQ(msgs.front().text,      "persisted text");
    EXPECT_EQ(msgs.front().timestamp, fixedTs);
}

TEST(ChatRoomAddMessageOverloadTest, AppendsAfterExistingMessages) {
    ChatRoom room("blend-1", {"user-1", "user-2"});

    room.addMessage("user-1", "first");                         // string overload
    room.addMessage(Message("user-2", "second", 9999999999LL)); // Message overload

    const std::list<Message> msgs = room.getMessages();
    ASSERT_EQ(msgs.size(), 2u);

    auto it = msgs.begin();
    EXPECT_EQ(it->userID, "user-1");
    ++it;
    EXPECT_EQ(it->userID,    "user-2");
    EXPECT_EQ(it->timestamp, static_cast<std::time_t>(9999999999LL));
}

// Live backend integration test

TEST(ChatHistoryIntegrationTest, MessageSentOverWebSocketAppearsInChatHistory) {
    skipUnlessLiveBackendEnabled();

    HttpClient client(kTestBackendBaseUrl);

    const std::string userID  = makeUniqueID("hist_user");
    const std::string blendID = makeUniqueID("hist_blend");

    // Register user
    const std::string regBody =
        "{\"user_id\":"  + ModelJson::quote(userID) +
        ",\"username\":\"hist tester\""
        ",\"email\":\"hist@test.local\""
        ",\"password\":\"pw123\"}";
    (void)client.post("/api/v1/auth/register", regBody);
    ASSERT_TRUE(HttpClient::isRequestSuccessful("POST", client.getLastStatusCode()))
        << "Registration failed";

    // Create blend
    const std::string blendBody =
        "{\"blend_id\":"   + ModelJson::quote(blendID) +
        ",\"creator_id\":" + ModelJson::quote(userID) +
        ",\"algorithm\":\"round_robin\""
        ",\"participants\":[" + ModelJson::quote(userID) + "]}";
    (void)client.post("/api/v1/blends", blendBody);
    ASSERT_TRUE(HttpClient::isRequestSuccessful("POST", client.getLastStatusCode()))
        << "Blend creation failed";

    // Send a message via WebSocket
    ix::initNetSystem();

    const std::string wsUrl = std::string(kTestBackendWsPrefix) + blendID + "?user_id=" + userID;

    std::mutex mu;
    std::condition_variable cv;
    bool opened = false;

    ix::WebSocket ws;
    ws.setUrl(wsUrl);
    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            std::lock_guard<std::mutex> lk(mu);
            opened = true;
            cv.notify_all();
        }
    });
    ws.start();

    {
        std::unique_lock<std::mutex> lk(mu);
        ASSERT_TRUE(cv.wait_for(lk, std::chrono::seconds(5), [&]{ return opened; }))
            << "WebSocket did not open in time";
    }

    const std::string sentContent = "history-test-msg-" + userID;
    ws.send("{\"content\":\"" + sentContent + "\"}");

    // Give the backend a moment to persist the message.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ws.stop();
    ix::uninitNetSystem();

    // Fetch history and verify the message appears.
    const std::string historyEndpoint =
        client.build_chat_history_endpoint(blendID, userID);
    const std::string historyResponse = client.get(historyEndpoint);
    ASSERT_TRUE(HttpClient::isRequestSuccessful("GET", client.getLastStatusCode()))
        << "GET /chat-history failed: " << historyResponse;

    std::list<Message> msgs = AppController::parseChatHistory(historyResponse);
    ASSERT_FALSE(msgs.empty()) << "History response was empty: " << historyResponse;

    bool found = false;
    for (const Message& m : msgs) {
        if (m.userID == userID && m.text == sentContent) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Sent message not found in history. Response: " << historyResponse;
}
