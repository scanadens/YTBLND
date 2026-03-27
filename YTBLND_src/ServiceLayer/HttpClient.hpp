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

class HttpClient {
	public:
		// Constant for posting to eleminate typinig errors
		const std::string P = "POST";
		// Constant for getting to eleminate typing errors
		const std::string G = "GET";

		/**
		 * Basic constructor. Assigns base url to this instance
		 * \param base string that will be saved to this instance
		 */
		HttpClient(std::string base);
		~HttpClient() = default;
		
		
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

	private:
		// Holds the base url for server communication
		std::string baseUrl;

		// used as a header internally (specifically for our server)
		const std::string hdr = "Content-Type: application/json";

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
		static size_t write_callback (void* contents, size_t size, size_t nmemb, std::string* out);
};