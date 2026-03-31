#include "gtest/gtest.h"

#include "../ServiceLayer/ChatWebSocket.hpp"
#include "../ServiceLayer/HttpClient.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>

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
    EXPECT_THROW(HttpClient::isRequestSuccessful("DELETE", 200), std::invalid_argument);
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
