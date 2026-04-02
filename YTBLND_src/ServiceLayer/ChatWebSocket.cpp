#include "ChatWebSocket.hpp"

#include <stdexcept>

using namespace std;

// Extracts the string value of a JSON key from a flat JSON object.
// Expects the format: "key":"value" — sufficient for the server's chat messages.
static std::string extractJsonStr(const std::string& json, const std::string& key) {
	std::string needle = "\"" + key + "\":\"";
	auto pos = json.find(needle);
	if (pos == std::string::npos) return "";
	pos += needle.size();
	auto end = json.find('"', pos);
	if (end == std::string::npos) return "";
	return json.substr(pos, end - pos);
}

ChatWebSocket::ChatWebSocket(const string url) {
	this->url = url;

	// setting up specialized broadcasting and functions
	ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            std::cout << "WS Received: " << msg->str << "\n";
            // Parse sender_id and content and forward to the registered callback.
            if (m_onMessage) {
                std::string sender  = extractJsonStr(msg->str, "sender_id");
                std::string content = extractJsonStr(msg->str, "content");
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
	stringstream ret;
	ret << "{\"content\": \"" << content << "\"}";
	return ret.str();
}

void ChatWebSocket::build_fullUrl() {
	// checks if blendID and userID have been correclty configured
	if (blendID.empty() || userID.empty()) {
		throw invalid_argument("ChatWebSocket requires blendID and userID before sending messages");
	}

	stringstream fu;
	fu << url << blendID << "?user_id=" << userID;
	fullUrl = fu.str(); 
}