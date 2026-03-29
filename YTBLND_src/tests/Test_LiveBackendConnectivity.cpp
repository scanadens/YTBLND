#include "gtest/gtest.h"

#include "../ServiceLayer/HttpClient.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>

namespace {

std::string makeUniqueID(const std::string& prefix) {
    // High-resolution timestamp avoids collisions when live tests are
    // re-run quickly against a persistent backend database.
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    const auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    std::ostringstream out;
    out << prefix << "_" << ticks;
    return out.str();
}

bool containsAll(const std::string& value, const std::initializer_list<std::string>& needles) {
    for (const std::string& needle : needles) {
        if (value.find(needle) == std::string::npos) {
            return false;
        }
    }
    return true;
}

void skipUnlessLiveBackendEnabled() {
    // This is an opt-in suite because it expects a real backend process
    // running at localhost:8080 with the documented API contract.
    const char* runLiveTests = std::getenv("YTBLND_RUN_LIVE_BACKEND_TESTS");
    if (!runLiveTests || std::string(runLiveTests) != "1") {
        GTEST_SKIP() << "Set YTBLND_RUN_LIVE_BACKEND_TESTS=1 and run the backend to execute this test.";
    }
}

void registerAndLogin(HttpClient& client, const std::string& userID) {
    // Register creates a durable user row used by subsequent blend/chat calls.
    const std::string registerBody =
        "{\"user_id\":\"" + userID +
        "\",\"username\":\"live tester\",\"email\":\"live@test.local\",\"password\":\"pw123\"}";
    const std::string registerResponse = client.post("/api/v1/auth/register", registerBody);
    ASSERT_TRUE(containsAll(registerResponse, {"user_id", userID, "username"})) << registerResponse;

    // Login verifies credential validation and confirms the same user identity
    // can be loaded back from backend storage.
    const std::string loginBody =
        "{\"user_id\":\"" + userID + "\",\"password\":\"pw123\"}";
    const std::string loginResponse = client.post("/api/v1/auth/login", loginBody);
    ASSERT_TRUE(containsAll(loginResponse, {"user_id", userID})) << loginResponse;
}

void createBlend(HttpClient& client, const std::string& blendID, const std::string& userID) {
    // Blend creation also provisions/syncs chat room membership in the backend.
    const std::string blendBody =
        "{\"blend_id\":\"" + blendID +
        "\",\"creator_id\":\"" + userID +
        "\",\"algorithm\":\"round_robin\",\"participants\":[\"" + userID + "\"]}";
    const std::string blendResponse = client.post("/api/v1/blends", blendBody);
    ASSERT_TRUE(containsAll(blendResponse, {"blend_id", blendID, "chat_room_id"})) << blendResponse;
}

}  // namespace

TEST(LiveBackendConnectivityTest, PingEndpointRespondsWithPong) {
    skipUnlessLiveBackendEnabled();

    // Fast health check that the live backend process is reachable.
    HttpClient client("http://localhost:8080");
    const std::string pingResponse = client.get("/ping");
    ASSERT_NE(pingResponse.find("pong"), std::string::npos) << pingResponse;
}

TEST(LiveBackendConnectivityTest, RegisterAndLoginRoundTripAgainstRunningServer) {
    skipUnlessLiveBackendEnabled();

    // Uses a per-test user ID so re-runs do not clash with prior DB state.
    HttpClient client("http://localhost:8080");
    const std::string userID = makeUniqueID("live_ws_user");

    registerAndLogin(client, userID);
}

TEST(LiveBackendConnectivityTest, CreateBlendReturnsExpectedChatRoomLink) {
    skipUnlessLiveBackendEnabled();

    // This test isolates persistence behavior before websocket concerns.
    HttpClient client("http://localhost:8080");
    const std::string userID = makeUniqueID("live_ws_user");
    const std::string blendID = makeUniqueID("live_ws_blend");

    registerAndLogin(client, userID);
    createBlend(client, blendID, userID);
}

TEST(LiveBackendConnectivityTest, WebSocketRoundTripAgainstRunningServer) {
    skipUnlessLiveBackendEnabled();

    // Full integration path: auth -> blend creation -> chatroom lookup -> WS.
    HttpClient client("http://localhost:8080");

    const std::string userID = makeUniqueID("live_ws_user");
    const std::string blendID = makeUniqueID("live_ws_blend");

    registerAndLogin(client, userID);
    createBlend(client, blendID, userID);

    const std::string chatRoomResponse =
        client.get("/api/v1/blends/" + blendID + "/chatroom");
    // Verifies that the user is actually attached to the room before we try
    // opening a websocket, so failures after this point are transport-focused.
    ASSERT_TRUE(containsAll(chatRoomResponse, {
        "blend_id",
        blendID,
        "chat_room_id",
        "member_users",
        userID
    })) << chatRoomResponse;

    ix::initNetSystem();

    std::mutex mu;
    std::condition_variable cv;
    std::string lastMessage;
    std::string openUri;
    std::string wsError;
    bool opened = false;
    bool gotMessage = false;

    const std::string wsUrl =
        "ws://localhost:8080/api/v1/ws/chats/" + blendID + "?user_id=" + userID;

    ix::WebSocket ws;
    ws.setUrl(wsUrl);
    ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg) {
        std::lock_guard<std::mutex> lock(mu);
        if (msg->type == ix::WebSocketMessageType::Open) {
            // Capture handshake metadata for contract validation.
            opened = true;
            openUri = msg->openInfo.uri;
            cv.notify_all();
            return;
        }
        if (msg->type == ix::WebSocketMessageType::Message) {
            // The backend echoes broadcast messages for room subscribers.
            gotMessage = true;
            lastMessage = msg->str;
            cv.notify_all();
            return;
        }
        if (msg->type == ix::WebSocketMessageType::Error) {
            // Preserve low-level socket error text for easier diagnosis.
            wsError = msg->errorInfo.reason;
            cv.notify_all();
        }
    });

    ws.start();

    {
        std::unique_lock<std::mutex> lock(mu);
        // Wait until either a successful open callback or an explicit error.
        const bool openReady = cv.wait_for(lock, std::chrono::seconds(5), [&]() {
            return opened || !wsError.empty();
        });
        ASSERT_TRUE(openReady) << "WebSocket did not open in time";
        ASSERT_TRUE(wsError.empty()) << "WebSocket failed to connect: " << wsError;
    }

    // ixwebsocket reports the opened request URI as path + query.
    const std::string expectedOpenUriPath =
        "/api/v1/ws/chats/" + blendID + "?user_id=" + userID;
    EXPECT_EQ(openUri, expectedOpenUriPath);

    const std::string payload = "{\"content\":\"hello from live connectivity test\"}";
    ws.send(payload);

    {
        std::unique_lock<std::mutex> lock(mu);
        // Wait for the server-side broadcast path to deliver at least one
        // room message back to this client connection.
        const bool msgReady = cv.wait_for(lock, std::chrono::seconds(5), [&]() {
            return gotMessage;
        });
        ASSERT_TRUE(msgReady) << "Did not receive chat broadcast from backend";
    }

    ws.stop();
    ix::uninitNetSystem();

    // Ensure the backend stamped trusted routing/identity fields into payload.
    EXPECT_TRUE(containsAll(lastMessage, {
        "chat_message",
        "sender_id",
        userID,
        "content",
        "hello from live connectivity test",
        "room_id",
        blendID
    })) << lastMessage;
}
