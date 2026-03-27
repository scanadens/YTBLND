#include "gtest/gtest.h"

#include "../ServiceLayer/HttpClient.hpp"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <string>

namespace {

std::string makeUniqueID(const std::string& prefix) {
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

}  // namespace

TEST(LiveBackendConnectivityTest, HttpAndWebSocketRoundTripAgainstRunningServer) {
    // This is an opt-in test because it expects a real backend process running
    // at localhost:8080 with the documented API contract.
    const char* runLiveTests = std::getenv("YTBLND_RUN_LIVE_BACKEND_TESTS");
    if (!runLiveTests || std::string(runLiveTests) != "1") {
        GTEST_SKIP() << "Set YTBLND_RUN_LIVE_BACKEND_TESTS=1 and run the backend to execute this test.";
    }

    HttpClient client("http://localhost:8080");

    const std::string pingResponse = client.get("/ping");
    ASSERT_NE(pingResponse.find("pong"), std::string::npos) << pingResponse;

    const std::string userID = makeUniqueID("live_ws_user");
    const std::string blendID = makeUniqueID("live_ws_blend");

    const std::string registerBody =
        "{\"user_id\":\"" + userID +
        "\",\"username\":\"live tester\",\"email\":\"live@test.local\",\"password\":\"pw123\"}";
    const std::string registerResponse = client.post("/api/v1/auth/register", registerBody);
    ASSERT_TRUE(containsAll(registerResponse, {"user_id", userID, "username"})) << registerResponse;

    const std::string loginBody =
        "{\"user_id\":\"" + userID + "\",\"password\":\"pw123\"}";
    const std::string loginResponse = client.post("/api/v1/auth/login", loginBody);
    ASSERT_TRUE(containsAll(loginResponse, {"user_id", userID})) << loginResponse;

    const std::string blendBody =
        "{\"blend_id\":\"" + blendID +
        "\",\"creator_id\":\"" + userID +
        "\",\"algorithm\":\"round_robin\",\"participants\":[\"" + userID + "\"]}";
    const std::string blendResponse = client.post("/api/v1/blends", blendBody);
    ASSERT_TRUE(containsAll(blendResponse, {"blend_id", blendID, "chat_room_id"})) << blendResponse;

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
            opened = true;
            openUri = msg->openInfo.uri;
            cv.notify_all();
            return;
        }
        if (msg->type == ix::WebSocketMessageType::Message) {
            gotMessage = true;
            lastMessage = msg->str;
            cv.notify_all();
            return;
        }
        if (msg->type == ix::WebSocketMessageType::Error) {
            wsError = msg->errorInfo.reason;
            cv.notify_all();
        }
    });

    ws.start();

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool openReady = cv.wait_for(lock, std::chrono::seconds(5), [&]() {
            return opened || !wsError.empty();
        });
        ASSERT_TRUE(openReady) << "WebSocket did not open in time";
        ASSERT_TRUE(wsError.empty()) << "WebSocket failed to connect: " << wsError;
    }

    EXPECT_EQ(openUri, wsUrl);

    const std::string payload = "{\"content\":\"hello from live connectivity test\"}";
    ws.send(payload);

    {
        std::unique_lock<std::mutex> lock(mu);
        const bool msgReady = cv.wait_for(lock, std::chrono::seconds(5), [&]() {
            return gotMessage;
        });
        ASSERT_TRUE(msgReady) << "Did not receive chat broadcast from backend";
    }

    ws.stop();
    ix::uninitNetSystem();

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
