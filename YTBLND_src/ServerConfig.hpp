#pragma once

/**
 * \file ServerConfig.hpp
 * \brief Centralized backend server addresses.
 * \author Shamar Pennant
 *
 * Edit the constants below to retarget the application and/or the test suite
 * to a different server.  No other file needs to change.
 *
 * - \c kApp* constants are used by the running application (AppController, UILayer).
 * - \c kTest* constants are used by the test suite and can point at a separate
 *   staging instance if needed.
 */

/** \brief HTTP base URL used by AppController and other app-side HTTP calls. */
inline constexpr const char* kAppBackendBaseUrl  = "http://137.220.58.22:8080";

/** \brief WebSocket endpoint prefix used by BlendChatPanel. */
inline constexpr const char* kAppBackendWsPrefix = "ws://137.220.58.22:8080/api/v1/ws/chats/";

/** \brief HTTP base URL used by the test suite. */
inline constexpr const char* kTestBackendBaseUrl  = "http://137.220.58.22:8080";

/** \brief WebSocket endpoint prefix used by tests that exercise WS connectivity. */
inline constexpr const char* kTestBackendWsPrefix = "ws://137.220.58.22:8080/api/v1/ws/chats/";
