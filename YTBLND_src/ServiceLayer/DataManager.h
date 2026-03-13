#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <optional>
#include <string>
#include <vector>
#include <list>
#include "../ModelLayer/User.h"
#include "../ModelLayer/Video.h"

/**
 * \brief Interface for all user-persistence operations.
 *
 * AppController depends only on this interface, keeping it decoupled from
 * the concrete storage back-end (SQLite today, something else tomorrow).
 * Tests can inject a mock implementation without touching the real DB.
 */
class DataManager {
public:
    virtual ~DataManager() = default;

    /**
     * Persists a new user record.
     * \return true on success, false if the userID already exists or the
     *         insert otherwise fails.
     */
    virtual bool createUser(const User& user) = 0;

    /**
     * Loads a user by their unique ID.
     * \return The User if found, std::nullopt otherwise.
     */
    virtual std::optional<User> findUserByID(const std::string& userID) = 0;

    /**
     * Checks whether the given plaintext password matches the stored record.
     * \return true if the userID exists and the password matches.
     */
    virtual bool validatePassword(const std::string& userID,
                                  const std::string& password) = 0;

    /**
     * Persists a user's Watch Later video list, replacing any previously saved list.
     * \return true on success.
     */
    virtual bool saveWatchLater(const std::string& userID,
                                const std::list<Video>& videos) = 0;

    /**
     * Loads the stored Watch Later video list for a user.
     * \return Empty list if no data has been saved for this user.
     */
    virtual std::list<Video> loadWatchLater(const std::string& userID) = 0;

    /**
     * Persists a blend and its participant list.
     * Re-calling with the same blendID replaces the existing record.
     * \param creatorID  userID of the user who initiated the blend.
     * \param participantIDs  All participant userIDs (including those without data).
     */
    virtual bool saveBlend(const std::string& blendID,
                           const std::string& creatorID,
                           const std::string& algorithm,
                           const std::vector<std::string>& participantIDs) = 0;

    /**
     * Returns the blendID of the blend this user belongs to, if any.
     * \return The blendID string, or std::nullopt if the user has no blend.
     */
    virtual std::optional<std::string> findBlendForUser(const std::string& userID) = 0;

    /**
     * Returns all participant userIDs for a given blend.
     */
    virtual std::vector<std::string> loadBlendParticipants(const std::string& blendID) = 0;
};

#endif
