#include "gtest/gtest.h"

#include "../ServiceLayer/ChatWebSocket.hpp"
#include "../ServiceLayer/HttpClient.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"
#include "../AppLayer/AppController.hpp"

#include <nlohmann/json.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace {

// Tiny single-threaded HTTP server used to test HttpClient without external
// backend dependencies. It only implements the routes needed by these tests.
class LocalHttpServer {
public:
    LocalHttpServer() {
        serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd_ < 0) {
            throw std::runtime_error("failed to create server socket");
        }

        int opt = 1;
        ::setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (::bind(serverFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(serverFd_);
            throw std::runtime_error("failed to bind server socket");
        }

        socklen_t len = sizeof(addr);
        if (::getsockname(serverFd_, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
            ::close(serverFd_);
            throw std::runtime_error("failed to inspect server socket port");
        }
        port_ = ntohs(addr.sin_port);

        if (::listen(serverFd_, 4) < 0) {
            ::close(serverFd_);
            throw std::runtime_error("failed to listen on server socket");
        }

        // Serve requests on a background thread so tests can use a normal
        // blocking HttpClient flow.
        running_.store(true);
        thread_ = std::thread([this]() { this->run(); });
    }

    ~LocalHttpServer() {
        running_.store(false);
        if (serverFd_ >= 0) {
            ::shutdown(serverFd_, SHUT_RDWR);
            ::close(serverFd_);
            serverFd_ = -1;
        }

        if (thread_.joinable()) {
            thread_.join();
        }
    }

    int port() const { return port_; }

private:
    void run() {
        while (running_.load()) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = ::accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (clientFd < 0) {
                if (!running_.load()) {
                    return;
                }
                continue;
            }

            handleClient(clientFd);
            ::close(clientFd);
        }
    }

    void handleClient(int clientFd) {
        std::string request;
        char buf[1024];

        // Read until we have headers.
        while (request.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
            if (n <= 0) {
                return;
            }
            request.append(buf, static_cast<size_t>(n));
        }

        const size_t headerEnd = request.find("\r\n\r\n");
        std::string headers = request.substr(0, headerEnd);
        std::string body = request.substr(headerEnd + 4);

        // Parse only request-line fields we need for route selection.
        std::string method;
        std::string path;
        {
            const size_t firstSpace = headers.find(' ');
            if (firstSpace == std::string::npos) {
                return;
            }
            const size_t secondSpace = headers.find(' ', firstSpace + 1);
            if (secondSpace == std::string::npos) {
                return;
            }
            method = headers.substr(0, firstSpace);
            path = headers.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        }

        // Read full POST payload based on Content-Length.
        size_t contentLength = 0;
        const std::string clHeader = "Content-Length:";
        size_t clPos = headers.find(clHeader);
        if (clPos != std::string::npos) {
            clPos += clHeader.size();
            while (clPos < headers.size() && headers[clPos] == ' ') {
                ++clPos;
            }
            size_t clEnd = headers.find("\r\n", clPos);
            contentLength = static_cast<size_t>(std::stoul(headers.substr(clPos, clEnd - clPos)));
        }

        while (body.size() < contentLength) {
            ssize_t n = ::recv(clientFd, buf, sizeof(buf), 0);
            if (n <= 0) {
                break;
            }
            body.append(buf, static_cast<size_t>(n));
        }

        std::string responseBody;
        std::string status;

        // Route table is intentionally minimal and deterministic for tests.
        if (method == "GET" && path == "/ping") {
            status = "HTTP/1.1 200 OK\r\n";
            responseBody = "{\"message\":\"pong\"}";
        } else if (method == "POST" && path == "/echo") {
            status = "HTTP/1.1 200 OK\r\n";
            responseBody = body;
        } else {
            status = "HTTP/1.1 404 Not Found\r\n";
            responseBody = "{}";
        }

        const std::string response = status +
            "Content-Type: application/json\r\n" +
            "Content-Length: " + std::to_string(responseBody.size()) + "\r\n" +
            "Connection: close\r\n\r\n" +
            responseBody;

        const char* data = response.data();
        size_t remaining = response.size();
        while (remaining > 0) {
            ssize_t sent = ::send(clientFd, data, remaining, 0);
            if (sent <= 0) {
                return;
            }
            data += sent;
            remaining -= static_cast<size_t>(sent);
        }
    }

    int serverFd_ = -1;
    int port_ = -1;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

}  // namespace

TEST(HttpClientWrapperTest, GetAndPostRoundTripAgainstLoopbackServer) {
    LocalHttpServer server;
    HttpClient client("http://127.0.0.1:" + std::to_string(server.port()));

    // Verifies GET wiring and response buffering.
    const std::string getResponse = client.get("/ping");
    EXPECT_NE(getResponse.find("pong"), std::string::npos);

    // Verifies POST body transport is not dropped or mangled.
    const std::string payload = "{\"hello\":\"world\"}";
    const std::string postResponse = client.post("/echo", payload);
    EXPECT_EQ(postResponse, payload);
}

TEST(HttpClientWrapperTest, StaticStatusHandlerMatchesFrontendContract) {
    EXPECT_TRUE(HttpClient::isRequestSuccessful("GET", 200));
    EXPECT_FALSE(HttpClient::isRequestSuccessful("GET", 201));
    EXPECT_FALSE(HttpClient::isRequestSuccessful("GET", 404));

    EXPECT_TRUE(HttpClient::isRequestSuccessful("POST", 200));
    EXPECT_TRUE(HttpClient::isRequestSuccessful("POST", 201));
    EXPECT_FALSE(HttpClient::isRequestSuccessful("POST", 400));

    EXPECT_TRUE(HttpClient::isRequestSuccessful("DELETE", 200));
    EXPECT_TRUE(HttpClient::isRequestSuccessful("DELETE", 204));
    EXPECT_FALSE(HttpClient::isRequestSuccessful("DELETE", 401));
}

TEST(HttpClientWrapperTest, LastStatusHelpersReflectMostRecentRequest) {
    LocalHttpServer server;
    HttpClient client("http://127.0.0.1:" + std::to_string(server.port()));

    (void)client.get("/ping");
    EXPECT_EQ(client.getLastStatusCode(), 200);
    EXPECT_TRUE(client.wasLastRequestSuccessful("GET"));

    const std::string payload = "{\"hello\":\"world\"}";
    (void)client.post("/echo", payload);
    EXPECT_EQ(client.getLastStatusCode(), 200);
    EXPECT_TRUE(client.wasLastRequestSuccessful("POST"));

    (void)client.get("/missing");
    EXPECT_EQ(client.getLastStatusCode(), 404);
    EXPECT_FALSE(client.wasLastRequestSuccessful("GET"));
}

TEST(HttpClientWrapperTest, StaticStatusHandlerRejectsUnsupportedMethods) {
    EXPECT_THROW(HttpClient::isRequestSuccessful("PATCH", 200), std::invalid_argument);
}

TEST(HttpClientWrapperTest, EndpointBuildersGenerateExpectedPaths) {
    HttpClient client("http://127.0.0.1:1");

    // Endpoint helpers are consumed by AppController and must match backend routes.
    EXPECT_EQ(
        client.build_watch_later_endpoint("user-123"),
        "/api/v1/users/user-123/watch-later"
    );
    EXPECT_EQ(
        client.build_latest_blend_endpoint("user-123"),
        "/api/v1/users/user-123/blend"
    );
    EXPECT_EQ(
        client.build_blend_participant_endpoint("blend-42"),
        "/api/v1/blends/blend-42/participants"
    );
    EXPECT_EQ(
        client.build_chatroom_detail_endpoint("blend-42"),
        "/api/v1/blends/blend-42/chatroom"
    );
    EXPECT_EQ(
        client.build_auth_user_lookup_endpoint("user-123"),
        "/api/v1/auth/users/user-123"
    );
    EXPECT_EQ(
        client.build_delete_user_endpoint("user-123"),
        "/api/v1/auth/users/user-123"
    );
}

TEST(RequestJsonBuilderTest, BuildLoginJsonProducesExpectedShape) {
    // Login payload contract: backend expects user_id + password.
    const std::string json = RequestJsonBuilder::buildLoginJson("user-1", "pw-1");
    const auto parsed = nlohmann::json::parse(json);

    EXPECT_EQ(parsed.at("user_id").get<std::string>(), "user-1");
    EXPECT_EQ(parsed.at("password").get<std::string>(), "pw-1");
}

TEST(RequestJsonBuilderTest, BuildRegisterJsonProducesExpectedShape) {
    // Register payload must include password to avoid server-side validation errors.
    const std::string json = RequestJsonBuilder::buildRegisterJson(
        "user-1",
        "alice",
        "alice@example.com",
        "pw-1"
    );
    const auto parsed = nlohmann::json::parse(json);

    EXPECT_EQ(parsed.at("user_id").get<std::string>(), "user-1");
    EXPECT_EQ(parsed.at("username").get<std::string>(), "alice");
    EXPECT_EQ(parsed.at("email").get<std::string>(), "alice@example.com");
    EXPECT_EQ(parsed.at("password").get<std::string>(), "pw-1");
}

TEST(RequestJsonBuilderTest, BuildBlendJsonProducesExpectedShape) {
    const std::vector<std::string> participants{"u1", "u2", "u3"};
    const std::string json = RequestJsonBuilder::buildBlendJson(
        "b1",
        "My Blend",
        "creator-1",
        "RandomBlendAlgorithm",
        participants
    );

    // Blend payload shape drives both blend creation and participant loading paths.
    const auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed.at("blend_id").get<std::string>(), "b1");
    EXPECT_EQ(parsed.at("title").get<std::string>(), "My Blend");
    EXPECT_EQ(parsed.at("creator_id").get<std::string>(), "creator-1");
    EXPECT_EQ(parsed.at("algorithm").get<std::string>(), "RandomBlendAlgorithm");
    ASSERT_TRUE(parsed.at("participants").is_array());
    EXPECT_EQ(parsed.at("participants").size(), 3u);
    EXPECT_EQ(parsed.at("participants").at(0).get<std::string>(), "u1");
    EXPECT_EQ(parsed.at("participants").at(1).get<std::string>(), "u2");
    EXPECT_EQ(parsed.at("participants").at(2).get<std::string>(), "u3");
}

TEST(RequestJsonBuilderTest, BuildWatchLaterJsonProducesExpectedShape) {
    std::list<Video> videos;
    videos.emplace_back("vid-1", "title-1", "", "", 0, std::list<std::string>{});
    videos.emplace_back("vid-2", "title-2", "", "", 0, std::list<std::string>{});

    // Watch-later sync serializes the video list under a top-level "videos" array.
    const std::string json = RequestJsonBuilder::buildWatchLaterJson(videos);
    const auto parsed = nlohmann::json::parse(json);

    ASSERT_TRUE(parsed.at("videos").is_array());
    ASSERT_EQ(parsed.at("videos").size(), 2u);
    EXPECT_EQ(parsed.at("videos").at(0).at("video_id").get<std::string>(), "vid-1");
    EXPECT_EQ(parsed.at("videos").at(0).at("title").get<std::string>(), "title-1");
    EXPECT_EQ(parsed.at("videos").at(1).at("video_id").get<std::string>(), "vid-2");
    EXPECT_EQ(parsed.at("videos").at(1).at("title").get<std::string>(), "title-2");
}

TEST(RequestJsonBuilderTest, BuildDeleteAccountJsonProducesExpectedShape) {
    const std::string json = RequestJsonBuilder::buildDeleteAccountJson("pw-1");
    const auto parsed = nlohmann::json::parse(json);

    EXPECT_EQ(parsed.at("password").get<std::string>(), "pw-1");
}

TEST(ChatWebSocketWrapperTest, MissingIDsThrowsValidationError) {
    // Wrapper must reject sends when required route parameters are absent.
    ChatWebSocket ws("ws://127.0.0.1:65530/api/v1/ws/chats/");
    EXPECT_THROW(ws.send_message("hello"), std::invalid_argument);
}

TEST(ChatWebSocketWrapperTest, BuildsExpectedSocketUrlBeforeSend) {
    ChatWebSocket ws("ws://127.0.0.1:65530/api/v1/ws/chats/");
    ws.set_blendID("blend-1");
    ws.set_userID("user-1");

    // No WS server is required here; we only assert URL assembly contract.
    EXPECT_NO_THROW(ws.send_message("hello"));
    EXPECT_EQ(
        ws.get_fullUrl(),
        "ws://127.0.0.1:65530/api/v1/ws/chats/blend-1?user_id=user-1"
    );

    // Let background socket thread process startup/connection failure cleanly.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


TEST(MetadataWorkflowTest, BoundedSubsetEnrichmentOnlyUpdatesFirstIncompleteVideos) {
    std::list<Video> videos;
    videos.emplace_back("v1", "", "", "", 0, std::list<std::string>{});
    videos.emplace_back("v2", "", "", "", 0, std::list<std::string>{});
    videos.emplace_back("v3", "", "", "", 0, std::list<std::string>{});
    videos.emplace_back("v4", "", "", "", 0, std::list<std::string>{});

    auto enrichChunk = [](const std::list<Video>& chunk) {
        std::list<Video> enriched;
        for (const auto& v : chunk) {
            enriched.emplace_back(
                v.getVideoID(),
                "Title-" + v.getVideoID(),
                "Channel-" + v.getVideoID(),
                "https://thumb/" + v.getVideoID(),
                v.getDuration(),
                v.getTags(),
                "Channel Name",
                "https://logo/" + v.getVideoID()
            );
        }
        return enriched;
    };

    const std::list<Video> result = AppController::enrichMissingMetadataSubsetMultithreaded(
        videos,
        2,
        3,
        enrichChunk
    );

    std::vector<Video> resultVec(result.begin(), result.end());
    ASSERT_EQ(resultVec.size(), 4u);
    EXPECT_EQ(resultVec[0].getTitle(), "Title-v1");
    EXPECT_EQ(resultVec[1].getTitle(), "Title-v2");
    EXPECT_TRUE(resultVec[2].getTitle().empty());
    EXPECT_TRUE(resultVec[3].getTitle().empty());
}

TEST(MetadataWorkflowTest, MultiThreadedPartitionAvoidsDuplicateWork) {
    std::list<Video> videos;
    for (int i = 1; i <= 9; ++i) {
        videos.emplace_back("id-" + std::to_string(i), "", "", "", 0, std::list<std::string>{});
    }

    std::mutex seenMutex;
    std::unordered_map<std::string, int> seenCounts;
    std::unordered_set<std::thread::id> workerThreadIds;

    auto enrichChunk = [&](const std::list<Video>& chunk) {
        std::list<Video> enriched;
        for (const auto& v : chunk) {
            {
                std::lock_guard<std::mutex> lock(seenMutex);
                seenCounts[v.getVideoID()]++;
                workerThreadIds.insert(std::this_thread::get_id());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            enriched.emplace_back(
                v.getVideoID(),
                "Title-" + v.getVideoID(),
                "Channel-" + v.getVideoID(),
                "https://thumb/" + v.getVideoID(),
                v.getDuration(),
                v.getTags(),
                "Channel Name",
                "https://logo/" + v.getVideoID()
            );
        }
        return enriched;
    };

    const std::list<Video> result = AppController::enrichMissingMetadataSubsetMultithreaded(
        videos,
        9,
        3,
        enrichChunk
    );

    for (const auto& kv : seenCounts) {
        EXPECT_EQ(kv.second, 1) << "Duplicate enrichment for video " << kv.first;
    }
    EXPECT_GE(workerThreadIds.size(), 2u);

    for (const auto& v : result) {
        EXPECT_FALSE(v.getTitle().empty());
        EXPECT_FALSE(v.getChannelID().empty());
    }
}
