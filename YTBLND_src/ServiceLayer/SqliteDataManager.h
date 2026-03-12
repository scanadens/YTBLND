#ifndef SQLITE_DATA_MANAGER_H
#define SQLITE_DATA_MANAGER_H

#include "DataManager.h"
#include <sqlite3.h>
#include <string>

/**
 * \brief SQLite-backed implementation of DataManager.
 *
 * Opens (or creates) a .db file on construction and runs the schema
 * migration automatically. Pass ":memory:" as the path to get an
 * in-memory database — useful for unit tests.
 */
class SqliteDataManager : public DataManager {
public:
    explicit SqliteDataManager(const std::string& dbPath);
    ~SqliteDataManager() override;

    bool createUser(const User& user)                              override;
    std::optional<User> findUserByID(const std::string& userID)   override;
    bool validatePassword(const std::string& userID,
                          const std::string& password)             override;

private:
    sqlite3* db = nullptr;

    // Runs CREATE TABLE IF NOT EXISTS on startup.
    void initSchema();
};

#endif
