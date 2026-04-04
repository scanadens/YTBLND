#pragma once

#include "../ServerConfig.hpp"
#include <ixwebsocket/IXWebSocket.h>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <string>
#include "HttpClient.hpp"

/**
 * \file ChatWebSocket.hpp
 * \author Shamar Pennant
 * \brief Simple WebSocket wrapper
 * 
 * Simple websocket wrapper to ease communications with the 
 * server in regards to chatrooms
 */

/**
 * \class ChatWebSocket
 * \brief ChatWebSocket class declaration.
 */
class ChatWebSocket {
	public:
	/**
	 * Sets up WebSocket to perform specialized actions upon successful
	 * connection, message receiving, and errors. Given the WebSocket
	 * remains connected after starting up until its been told to stop
	 * listening for responses
	 * \param url Is assigned to this->url
	 */
	ChatWebSocket(const std::string url);

	/**
	 * Stops the WebSocket from listening for responses.
	 */
	~ChatWebSocket();

	/**
	 * Opens the WebSocket connection without sending a message.
	 * Must be called after set_blendID() and set_userID() are set.
	 * No-op if already connected.
	 */
	void connect();

	/**
	 * Registers a callback invoked (on the WebSocket background thread)
	 * whenever a chat message is received from the server.
	 * \param cb Called with (senderID, messageContent).
	 */
	void setOnMessage(std::function<void(const std::string& senderID,
	                                     const std::string& content)> cb);

	/**
	 * Sets blendID
	 * \param blendID the blend id to be set
	 */
	void set_blendID(const std::string blendID);

	/**
	 * Sets userID
	 * \param userID the user id to be set
	 */
	void set_userID(const std::string userID);

	/**
	 * Retrieves the full websocket url
	 * \return string of currently configured websocket url
	 */
	std::string get_fullUrl();

	/**
	 * Sends a message through the WebSocket (given the url
	 * has been correctly configured with blend and user ID)
	 * \param msg String message (leave as plain string, no formatting needed)
	 */
	void send_message(std::string msg);

	private:
	// Websocket object
	ix::WebSocket ws;
	// copy of the last given url
	std::string url = kAppBackendWsPrefix;
	// copy of the last given blendID for the socket path
	std::string blendID;
	// copy of the last given userID for the socket path
	std::string userID;
	// full websocket url
	std::string fullUrl;
	// Tracks whether the socket loop has been started.
	bool isStarted = false;
	// Optional callback invoked on incoming chat messages.
	std::function<void(const std::string&, const std::string&)> m_onMessage;

	/**
	 * Formats string into appropriate JSON structure as needed 
	 * by the server.
	 * \param content String to be formatted
	 * \return the structure JSON form of content ("({"content": <content>}"))
	 */
	std::string content_formatter(std::string content);

	/**
	 * helper function that builds the full url path for the socket to 
	 * send messages to
	 */
	void build_fullUrl();
};
