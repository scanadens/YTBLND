#include "gtest/gtest.h"

#include "../ServerConfig.hpp"
#include "../ServiceLayer/HttpClient.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

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

std::string getenvOrDefault(const char* key, const std::string& fallback) {
    const char* value = std::getenv(key);
    if (!value || std::string(value).empty()) {
        return fallback;
    }
    return std::string(value);
}

std::string deriveWsEndpointPrefixFromHttpBase(const std::string& httpBaseUrl) {
    std::string wsBase = httpBaseUrl;
    const std::string httpsPrefix = "https://";
    const std::string httpPrefix = "http://";

    if (wsBase.rfind(httpsPrefix, 0) == 0) {
        wsBase.replace(0, httpsPrefix.size(), "wss://");
    } else if (wsBase.rfind(httpPrefix, 0) == 0) {
        wsBase.replace(0, httpPrefix.size(), "ws://");
    }

    while (!wsBase.empty() && wsBase.back() == '/') {
        wsBase.pop_back();
    }

    return wsBase + "/api/v1/ws/chats/";
}

std::string pathAndQueryFromUrl(const std::string& url) {
    const std::size_t schemePos = url.find("://");
    if (schemePos == std::string::npos) {
        return url;
    }

    const std::size_t pathPos = url.find('/', schemePos + 3);
    if (pathPos == std::string::npos) {
        return "/";
    }
    return url.substr(pathPos);
}

bool containsAll(const std::string& value, const std::initializer_list<std::string>& needles) {
    for (const std::string& needle : needles) {
        if (value.find(needle) == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool isLiveBackendEnabled() {
    // Explicit opt-out always wins.
    const char* skipLiveTests = std::getenv("YTBLND_SKIP_LIVE_BACKEND_TESTS");
    if (skipLiveTests && std::string(skipLiveTests) == "1") {
        return false;
    }

    // Verify the configured server is actually reachable so tests skip
    // gracefully instead of failing when the server is down.
    HttpClient probe(kTestBackendBaseUrl);
    try {
        const std::string resp = probe.get("/ping");
        return probe.wasLastRequestSuccessful(probe.G) &&
               resp.find("pong") != std::string::npos;
    } catch (...) {
        return false;
    }
}

std::string waitForChatRoomDetails(HttpClient& client, const std::string& blendID, const std::string& userID) {
    std::string lastResponse;
    for (int attempt = 0; attempt < 10; ++attempt) {
        lastResponse = client.get("/api/v1/blends/" + blendID + "/chatroom");
        if (containsAll(lastResponse, {"blend_id", blendID, "chat_room_id", "member_users", userID})) {
            return lastResponse;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return lastResponse;
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

void deleteUser(HttpClient& client, const std::string& userID, const std::string& password) {
    const std::string deleteBody =
        "{\"password\":\"" + password + "\"}";
    const std::string deleteResponse =
        client.del("/api/v1/auth/users/" + userID, deleteBody);

    ASSERT_TRUE(client.wasLastRequestSuccessful(client.D))
        << "Expected successful account deletion, got HTTP "
        << client.getLastStatusCode() << " response=" << deleteResponse;
    EXPECT_TRUE(containsAll(deleteResponse, {"status", "deleted", "user_id", userID}))
        << deleteResponse;
}

}  // namespace

TEST(LiveBackendConnectivityTest, PingEndpointRespondsWithPong) {
    if (!isLiveBackendEnabled()) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }

    // Fast health check that the live backend process is reachable.
    HttpClient client(kTestBackendBaseUrl);
    const std::string pingResponse = client.get("/ping");
    ASSERT_NE(pingResponse.find("pong"), std::string::npos) << pingResponse;
}

TEST(LiveBackendConnectivityTest, RegisterAndLoginRoundTripAgainstRunningServer) {
    if (!isLiveBackendEnabled()) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }

    // Uses a per-test user ID so re-runs do not clash with prior DB state.
    HttpClient client(kTestBackendBaseUrl);
    const std::string userID = makeUniqueID("live_ws_user");

    registerAndLogin(client, userID);
}

TEST(LiveBackendConnectivityTest, DeleteAccountRemovesUserFromAuthLoginPath) {
    if (!isLiveBackendEnabled()) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }

    HttpClient client(kTestBackendBaseUrl);
    const std::string userID = makeUniqueID("live_delete_user");
    const std::string password = "pw123";

    registerAndLogin(client, userID);
    deleteUser(client, userID, password);

    // A deleted account must no longer authenticate through /auth/login.
    const std::string loginBody =
        "{\"user_id\":\"" + userID + "\",\"password\":\"" + password + "\"}";
    const std::string loginAfterDelete = client.post("/api/v1/auth/login", loginBody);
    EXPECT_FALSE(client.wasLastRequestSuccessful(client.P))
        << "Deleted user unexpectedly logged in: " << loginAfterDelete;
    EXPECT_EQ(client.getLastStatusCode(), client.INV_USR_LOG_ERR)
        << "Expected 401 after deleting account; response=" << loginAfterDelete;
}

TEST(LiveBackendConnectivityTest, CreateBlendReturnsExpectedChatRoomLink) {
    if (!isLiveBackendEnabled()) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }

    // This test isolates persistence behavior before websocket concerns.
    HttpClient client(kTestBackendBaseUrl);
    const std::string userID = makeUniqueID("live_ws_user");
    const std::string blendID = makeUniqueID("live_ws_blend");

    registerAndLogin(client, userID);
    createBlend(client, blendID, userID);
}

TEST(LiveBackendConnectivityTest, WebSocketRoundTripAgainstRunningServer) {
    if (!isLiveBackendEnabled()) {
        GTEST_SKIP() << "Live backend unreachable at " << kTestBackendBaseUrl;
    }

    // Full integration path: auth -> blend creation -> chatroom lookup -> WS.
    HttpClient client(kTestBackendBaseUrl);
    const std::string wsEndpointPrefix = kTestBackendWsPrefix;

    const std::string userID = makeUniqueID("live_ws_user");
    const std::string blendID = makeUniqueID("live_ws_blend");

    registerAndLogin(client, userID);
    createBlend(client, blendID, userID);

    const std::string chatRoomResponse = waitForChatRoomDetails(client, blendID, userID);
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
    int wsErrorStatus = 0;
    uint32_t wsRetries = 0;
    bool opened = false;
    bool gotMessage = false;

    const std::string wsUrl = wsEndpointPrefix + blendID + "?user_id=" + userID;

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
            wsErrorStatus = msg->errorInfo.http_status;
            wsRetries = msg->errorInfo.retries;
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
        ASSERT_TRUE(wsError.empty())
            << "WebSocket failed to connect"
            << " | ws_url=" << wsUrl
            << " | http_base=" << kTestBackendBaseUrl
            << " | ws_http_status=" << wsErrorStatus
            << " | retries=" << wsRetries
            << " | blend_id=" << blendID
            << " | user_id=" << userID
            << " | chatroom_probe=" << chatRoomResponse
            << " | error_reason=" << wsError;
    }

    // ixwebsocket reports the opened request URI as path + query.
    const std::string expectedOpenUriPath = pathAndQueryFromUrl(wsUrl);
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
