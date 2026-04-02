/**
 * test implementation of http client wrapper for centralized use.
 * should act as a singleton within each app instace to avoid 
 * deadlocks or livelocks
 */

#include "HttpClient.hpp"

#include <stdexcept>

using namespace std;

namespace {

string urlEncodeComponent(const string& value) {
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

	string encoded(escaped);
	curl_free(escaped);
	curl_easy_cleanup(encoder);
	return encoded;
}

string sanitizePathSegment(const string& value) {
	return urlEncodeComponent(value);
}

}

HttpClient::HttpClient(string base) {
	baseUrl = move(base);
}

bool HttpClient::isRequestSuccessful(const string& method, long statusCode) {
	if (method == "GET") {
		return statusCode == 200;
	}
	if (method == "POST") {
		return statusCode == 200 || statusCode == 201;
	}
	if (method == "DELETE") {
		return statusCode == 200 || statusCode == 204;
	}

	throw invalid_argument("HttpClient::isRequestSuccessful received unsupported method: " + method);
}

long HttpClient::getLastStatusCode() const {
	return lastStatusCode;
}

bool HttpClient::wasLastRequestSuccessful(const string& method) const {
	return isRequestSuccessful(method, lastStatusCode);
}

string HttpClient::get(const string& path) {
	return request(G, path, "");
}

string HttpClient::post(const string& path, const string& json) {
	return request(P, path, json);
}

string HttpClient::del(const string& path, const string& json) {
	return request(D, path, json);
}

string HttpClient::build_watch_later_endpoint(string userID) {
	return "/api/v1/users/" + sanitizePathSegment(userID) + "/watch-later";
}

string HttpClient::build_latest_blend_endpoint(string userID) {
	return "/api/v1/users/" + sanitizePathSegment(userID) + "/blend";
}

string HttpClient::build_blend_participant_endpoint(string blendID) {
	return "/api/v1/blends/" + sanitizePathSegment(blendID) + "/participants";
}

string HttpClient::build_chatroom_detail_endpoint(string blendID) {
	return "/api/v1/blends/" + sanitizePathSegment(blendID) + "/chatroom";
}

string HttpClient::build_auth_user_lookup_endpoint(string userID) {
	return AUTH_USER_PREFIX + sanitizePathSegment(userID);
}

string HttpClient::build_delete_user_endpoint(string userID) {
	return DELETE_USER_PREFIX + sanitizePathSegment(userID);
}

string HttpClient::request(const string& method, const string& path, const string&body) {
	// grabbing a new curl object
	CURL* curl = curl_easy_init();
	if (!curl) {
		throw runtime_error("HttpClient failed to initialize CURL handle");
	}

	// string containing the response from the HTTP method
	string response;
	// full path to endpoint
	const string fullUrl = baseUrl + path;

	// --- setting up curl for requests ---

	// instruct curl where to send the the request
	curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
	// curl doesnt return the call directly, so we need write_callback()
	// before being able to grabe the response
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	// create the header for our request (following curl design patterns)
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, hdr.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	

	// --- handling given method ---
	if (method == P) { // POST
		// send the given body
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));

	} else if (method == G) { // GET
		/**
		 * just have this here for comepleteness
		 * libcurl will by default request through GET.
		 * but with the above cmd, libcurl will 
		 * switch to POST automatically
		 * 
		 * within the future can add extra blocks for 
		 * other request methods
		 */

		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	} else if (method == D) { // DELETE
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, D.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
	} else {
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		throw invalid_argument("HttpClient::request received unsupported method: " + method);
	}

	// firing off the request and clean up all pointers
	CURLcode result = curl_easy_perform(curl);

	long statusCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
	lastStatusCode = statusCode;

	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);

	if (result != CURLE_OK) {
		throw runtime_error(string("HttpClient request failed: ") + curl_easy_strerror(result));
	}

	return response;
}

string HttpClient::build_user_blends_endpoint(string userID) {
	return "/api/v1/users/" + sanitizePathSegment(userID) + "/blends";
}

string HttpClient::build_leave_blend_endpoint(string blendID) {
	return "/api/v1/blends/" + sanitizePathSegment(blendID) + "/leave";
}

string HttpClient::build_chat_history_endpoint(string blendID, string userID) {
	return "/api/v1/blends/" + sanitizePathSegment(blendID) + "/chat-history?user_id=" + sanitizePathSegment(userID);
}

size_t HttpClient::write_callback(void* contents, size_t size, size_t nmemb, void* out) {
	auto* response = static_cast<string*>(out);
	response->append(static_cast<char*>(contents), size * nmemb);

	return size * nmemb;
}