/**
 * \file ChatWebSocket.cpp
 * \brief Implementation for ChatWebSocket.
 * \author Shamar Pennant
 */

#include "ChatWebSocket.hpp"
#include "../ModelLayer/JsonUtils.hpp"

#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace std;
using Json = nlohmann::json;

static std::string urlEncode(const std::string& value) {
	if (value.empty()) {
		return value;
	}

	CURL* encoder = curl_easy_init();
	if (encoder == nullptr) {
		return value;
	}

	char* escaped = curl_easy_escape(encoder, value.c_str(), static_cast<int>(value.size()));
	if (escaped == nullptr) {
		curl_easy_cleanup(encoder);
		return value;
	}

	std::string encoded(escaped);
	curl_free(escaped);
	curl_easy_cleanup(encoder);
	return encoded;
}

ChatWebSocket::ChatWebSocket(const string url) {
	this->url = url;

	// setting up specialized broadcasting and functions
	ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "WS Received: " << msg->str << "\n";
			// Parse sender/content from contract-shaped outbound server messages.
            if (m_onMessage) {
				Json parsed = Json::parse(msg->str, nullptr, false);
				if (parsed.is_discarded() || !parsed.is_object()) {
					return;
				}

				std::string sender  = parsed.value("sender_id", "");
				std::string content = parsed.value("content", "");
                if (!sender.empty() && !content.empty()) {
                    m_onMessage(sender, content);
                }
            }
        }
        else if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << "WS Connected\n";
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
            std::cerr << "WS Error: " << msg->errorInfo.reason << "\n";
        }
	});
}

ChatWebSocket::~ChatWebSocket() {
	if (isStarted) {
		ws.stop();
	}
}

void ChatWebSocket::set_blendID(const string blendID) { this->blendID = blendID; }
void ChatWebSocket::set_userID(const string userID) { this->userID = userID; }
string ChatWebSocket::get_fullUrl() { return fullUrl; }

void ChatWebSocket::connect() {
	if (isStarted) return;
	build_fullUrl();
	ws.setUrl(fullUrl);
	ws.start();
	isStarted = true;
}

void ChatWebSocket::setOnMessage(
        std::function<void(const std::string&, const std::string&)> cb) {
	m_onMessage = std::move(cb);
}

void ChatWebSocket::send_message(string msg) {
	connect(); // no-op if already started
	ws.send(content_formatter(msg));
}

string ChatWebSocket::content_formatter(string content) {
	return "{\"content\":" + ModelJson::quote(content) + "}";
}

void ChatWebSocket::build_fullUrl() {
	// checks if blendID and userID have been correclty configured
	if (blendID.empty() || userID.empty()) {
		throw invalid_argument("ChatWebSocket requires blendID and userID before sending messages");
	}

	stringstream fu;
	fu << url << urlEncode(blendID) << "?user_id=" << urlEncode(userID);
	fullUrl = fu.str(); 
}