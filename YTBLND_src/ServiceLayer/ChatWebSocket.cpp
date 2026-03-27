#include "ChatWebSocket.hpp"

#include <stdexcept>

using namespace std;

ChatWebSocket::ChatWebSocket(const string url) {
	this->url = url;

	// setting up specialized broadcasting and functions
	ws.setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) { 
			// upon reception
            cout << "WS Received: " << msg->str << "\n";
        }
        else if (msg->type == ix::WebSocketMessageType::Open) {
			// upon successful connection
            cout << "WS Connected\n";
        }
        else if (msg->type == ix::WebSocketMessageType::Error) {
			// upon error
            cerr << "WS Error: " << msg->errorInfo.reason << "\n";
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

void ChatWebSocket::send_message(string msg) {
	build_fullUrl();
	if (!isStarted) {
		ws.setUrl(fullUrl);
		ws.start();
		isStarted = true;
	}

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