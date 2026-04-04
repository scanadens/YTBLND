/**
 * \file RequestJsonBuilder.hpp
 * \brief JSON request body builders used by ServiceLayer HTTP calls.
 * \author Shamar Pennant
 */

#pragma once

#include "../ModelLayer/Video.hpp"

#include <list>
#include <string>
#include <vector>

namespace RequestJsonBuilder {

/**
 * \brief Builds the registration request body for the auth/register endpoint.
 *
 * Output format:
 * \code{.json}
 * {
 *   "user_id": "...",
 *   "username": "...",
 *   "email": "...",
 *   "password": "..."
 * }
 * \endcode
 *
 * \param userID User identifier chosen by the registering user.
 * \param username Display name to store with the account.
 * \param email Optional email value (can be empty).
 * \param password Plain-text password entered during registration.
 * \return JSON string safe for HTTP POST to the register endpoint.
 */
std::string buildRegisterJson(const std::string& userID,
                              const std::string& username,
                              const std::string& email,
                              const std::string& password);

/**
 * \brief Builds the login request body for the auth/login endpoint.
 *
 * Output format:
 * \code{.json}
 * { "user_id": "...", "password": "..." }
 * \endcode
 *
 * \param userID User identifier provided by the client.
 * \param password Plain-text password entered by the user.
 * \return JSON string safe for HTTP POST to the login endpoint.
 */
std::string buildLoginJson(const std::string& userID, const std::string& password);

/**
 * \brief Builds the delete-account request body.
 *
 * Output format:
 * \code{.json}
 * { "password": "..." }
 * \endcode
 *
 * \param password Plain-text password entered by the user for re-authentication.
 * \return JSON string safe for HTTP DELETE to the user-delete endpoint.
 */
std::string buildDeleteAccountJson(const std::string& password);

/**
 * \brief Builds the blend creation request body.
 *
 * Output format:
 * \code{.json}
 * {
 *   "blend_id": "...",
 *   "creator_id": "...",
 *   "algorithm": "...",
 *   "participants": ["...", "..."]
 * }
 * \endcode
 *
 * \param blendID Unique blend identifier generated client-side.
 * \param creatorID User ID of the blend creator.
 * \param algorithm Name of the algorithm used to generate the blend.
 * \param participants Ordered participant user IDs for this blend.
 * \return JSON string safe for HTTP POST to the blend endpoint.
 */
std::string buildBlendJson(const std::string& blendID,
                           const std::string& title,
                           const std::string& creatorID,
                           const std::string& algorithm,
                           const std::vector<std::string>& participants);

/**
 * \brief Builds the watch-later persistence request body.
 *
 * Output format:
 * \code{.json}
 * {
 *   "videos": [
 *     { "video_id": "...", "title": "..." }
 *   ]
 * }
 * \endcode
 *
 * \param videos Watch-later videos to serialize.
 * \return JSON string safe for HTTP POST to the watch-later endpoint.
 */
std::string buildWatchLaterJson(const std::list<Video>& videos);

} // namespace RequestJsonBuilder
