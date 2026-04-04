#pragma once

#include <curl/curl.h>
#include <string>
#include <iostream>
#include <memory>
#include <stdio.h>

/**
 * \file HttpClient.hpp
 * \author Shamar Pennant
 * \brief Simple HTTP client wrapper
 * 
 * Handles sending and receiving HTTP messages to our custom server. Using custom formatting 
 * and structure as defined in the route_layer/
 */

/**
 * \class HttpClient
 * \brief HttpClient class declaration.
 */
class HttpClient {
	public:
		// Constant for posting to eliminate typing errors
		const std::string P = "POST";
		// Constant for getting to eliminate typing errors
		const std::string G = "GET";
		// Constant for deleting to eliminate typing errors
		const std::string D = "DELETE";

		// --- endpoint constants ---

		// user registration endpoint
		const std::string REG_USER = "/api/v1/auth/register";
		// user login endpoint
		const std::string LOGIN = "/api/v1/auth/login";
		// user profile lookup endpoint prefix
		const std::string AUTH_USER_PREFIX = "/api/v1/auth/users/";
		// user delete endpoint prefix
		const std::string DELETE_USER_PREFIX = "/api/v1/auth/users/";
		// endpoint pertaining to creating a blend
		const std::string BLEND = "/api/v1/blends";

		// --- status codes ---
		
		const int SUCC = 200; // success
		const int ALT_SUCC = 201; // alt success
		const int MAL_REQ_ERR = 400; // malformed request error
		const int INV_USR_LOG_ERR = 401; // invalid login credentials
		const int UNAUTH_RES_REQ_ERR = 403; // user not authorized for request resource
		const int MISS_RM_CNTXT = 404; // missing user or blend room context
		const int DUPLICATE = 409; // duplicate entry
		const int SERVER_ERR = 500; // server error
		//

		/**
		 * Basic constructor. Assigns base url to this instance
		 * \param base string that will be saved to this instance
		 */
		HttpClient(std::string base);
		~HttpClient() = default;

		/**
		 * \brief Evaluates whether an HTTP status code should be treated as a
		 * successful result for a specific request method.
		 *
		 * This is the central error-code handler for HttpClient. Use it whenever
		 * you need to decide if a completed request succeeded semantically, not
		 * just transport-wise.
		 *
		 * Success rules are intentionally aligned with the frontend/backend
		 * contract in `Docs/Front_End_Requirements_for_Server_Communication.md`:
		 * - `GET` is successful only for `200 OK`.
		 * - `POST` is successful for `200 OK` or `201 Created`.
		 * - All other status codes are considered failures.
		 *
		 * \param method HTTP verb string (`"GET"` or `"POST"`).
		 * \param statusCode Numeric HTTP status code returned by the server.
		 * \return `true` when the code represents success for the given method;
		 *         `false` otherwise.
		 * \throws std::invalid_argument if the method is unsupported.
		 */
		static bool isRequestSuccessful(const std::string& method, long statusCode);

		/**
		 * \brief Returns the HTTP status code from the most recent successful
		 * libcurl transfer initiated by this client instance.
		 *
		 * This value is updated by `request()` after `curl_easy_perform()` and is
		 * available to callers who need post-call diagnostics.
		 *
		 * \return Last response status code, or `0` before any request has run.
		 */
		long getLastStatusCode() const;

		/**
		 * \brief Convenience helper that applies `isRequestSuccessful()` to the
		 * latest response code captured by this client.
		 *
		 * \param method HTTP verb to evaluate (`"GET"` or `"POST"`).
		 * \return `true` when the latest response code is successful for `method`.
		 * \throws std::invalid_argument if the method is unsupported.
		 */
		bool wasLastRequestSuccessful(const std::string& method) const;
		
		
		/**
		 * Public facing get method. Aquires the formatted string based on the given path endpoint.
		 * Helps simplify pinging the server for data by providing the 
		 * \param path string endpoint path pointer the formatted string is for
		 * \return formatted json string
		 */
		std::string get(const std::string& path);
		/**
		 * Public facing post method. fires HTTP message to the server.
		 * \param path the endpoint string pointer
		 * \param json formatted json string pointer message to be sent
		 * \return 
		 */
		std::string post(const std::string& path, const std::string& json);

		/**
		 * Public facing delete method. Fires HTTP DELETE message to the server.
		 * \param path endpoint string pointer
		 * \param json formatted json string pointer message to be sent
		 * \return server response body
		 */
		std::string del(const std::string& path, const std::string& json);

		/**
		 * \brief Builds the watch-later endpoint for a specific user.
		 *
		 * Result format: \c /api/v1/users/{userID}/watch-later
		 *
		 * \param userID User identifier used in the endpoint path.
		 * \return Endpoint path string for persisting or retrieving watch-later data.
		 */
		std::string build_watch_later_endpoint(std::string userID);

		/**
		 * \brief Builds the endpoint used to fetch a user's latest blend.
		 *
		 * Result format: \c /api/v1/users/{userID}/blend
		 *
		 * \param userID User identifier used in the endpoint path.
		 * \return Endpoint path string for latest blend lookup.
		 */
		std::string build_latest_blend_endpoint(std::string userID);

		/**
		 * \brief Builds the endpoint used to fetch participant IDs for a blend.
		 *
		 * Result format: \c /api/v1/blends/{blendID}/participants
		 *
		 * \param blendID Blend identifier used in the endpoint path.
		 * \return Endpoint path string for participant lookup.
		 */
		std::string build_blend_participant_endpoint(std::string blendID);

		/**
		 * \brief Builds the endpoint used to fetch chat room details for a blend.
		 *
		 * Result format: \c /api/v1/blends/{blendID}/chatroom
		 *
		 * \param blendID Blend identifier used as the chatroom context.
		 * \return Endpoint path string for chatroom details.
		 */
		std::string build_chatroom_detail_endpoint(std::string blendID);

		/**
		 * \brief Builds the auth user lookup endpoint for a specific user.
		 *
		 * Result format: \c /api/v1/auth/users/{userID}
		 *
		 * This endpoint is the canonical account-existence lookup in the
		 * frontend/backend contract and should be used instead of feature routes
		 * like watch-later when validating participant IDs.
		 *
		 * \param userID User identifier used in the endpoint path.
		 * \return Endpoint path string for user profile lookup.
		 */
		std::string build_auth_user_lookup_endpoint(std::string userID);

		/**
		 * \brief Builds the account-delete endpoint for a specific user.
		 *
		 * Result format: \c /api/v1/auth/users/{userID}
		 *
		 * \param userID User identifier used in the endpoint path.
		 * \return Endpoint path string for account deletion.
		 */
		std::string build_delete_user_endpoint(std::string userID);

		/**
		 * Builds the endpoint for fetching all blends a user participates in.
		 * Result format: \c /api/v1/users/{userID}/blends
		 * \param userID User identifier.
		 * \return Endpoint path string for listing user blends.
		 */
		std::string build_user_blends_endpoint(std::string userID);

		/**
		 * Builds the endpoint for leaving a blend.
		 * Result format: \c /api/v1/blends/{blendID}/leave
		 * \param blendID Blend identifier.
		 * \return Endpoint path string for leaving a blend.
		 */
		std::string build_leave_blend_endpoint(std::string blendID);

		/**
		 * Builds the endpoint for fetching a blend's chat message history.
		 * \param blendID ID of the blend whose chat history is requested.
		 * \param userID  ID of the requesting user (passed as query param).
		 * \return the structured string endpoint
		 */
		std::string build_chat_history_endpoint(std::string blendID, std::string userID);

	private:
		// Holds the base url for server communication
		std::string baseUrl;

		// used as a header internally (specifically for our server)
		const std::string hdr = "Content-Type: application/json";

		// HTTP status code from the latest response (0 means not set yet).
		long lastStatusCode = 0;

		/**
		 * Helper function. Does the grunt work of retrieving and posting data to the server
		 * \param method Determines whether or not we POST or GET
		 * \param path determine the endpoint we need to use
		 * \param body the JSON body that needs to be POSTed. leave empty when GETting
		 * \return the response from either operation
		 */
		std::string request(const std::string& method, const std::string& path, const std::string& body);

		/**
		 * Part of the libcurl design pattern. Used to determine how streamed
		 * data should be handled from the request response. (libcurl does not handle 
		 * how to deal with streamed data)
		 * \param contents Is the pointer to a chunk of raw bytes from the server
		 * \param size Describes how long the chunk is
		 * \param nmemb Aids in describing the size of the chunk (like typical malloc patterns)
		 * \param out A pointer to a string we would like to append the chunks to (in this case)
		 * \return (size * nmemb) as part of the libcurl pattern to verify data handling from callback
		 */
		static size_t write_callback(void* contents, size_t size, size_t nmemb, void* out);
};
