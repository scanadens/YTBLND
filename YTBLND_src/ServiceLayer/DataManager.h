#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <optional>
#include <string>
#include "../ModelLayer/User.h"

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
};

#endif
