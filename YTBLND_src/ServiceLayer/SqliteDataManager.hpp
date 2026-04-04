#ifndef SQLITE_DATA_MANAGER_H
#define SQLITE_DATA_MANAGER_H

#include "DataManager.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <list>

/**
 * \file SqliteDataManager.hpp
 * \brief SQLite-backed implementation of DataManager.
 * \author Jasmine Kumar
 *
 * Opens (or creates) a .db file on construction and runs the schema
 * migration automatically. Pass ":memory:" as the path to get an
 * in-memory database - useful for unit tests.
 */
/**
 * \class SqliteDataManager
 * \brief SqliteDataManager type declaration.
 */
class SqliteDataManager : public DataManager {
public:
    explicit SqliteDataManager(const std::string& dbPath);
    ~SqliteDataManager() override;

    bool createUser(const User& user)                              override;
    std::optional<User> findUserByID(const std::string& userID)   override;
    bool validatePassword(const std::string& userID,
                          const std::string& password)             override;

    bool saveWatchLater(const std::string& userID,
                        const std::list<Video>& videos)            override;
    std::list<Video> loadWatchLater(const std::string& userID)     override;

    bool saveBlend(const std::string& blendID,
                   const std::string& creatorID,
                   const std::string& algorithm,
                   const std::vector<std::string>& participantIDs) override;
    std::optional<std::string> findBlendForUser(
                   const std::string& userID)                      override;
    std::vector<std::string> loadBlendParticipants(
                   const std::string& blendID)                     override;

private:
    sqlite3* db = nullptr;

    // Runs CREATE TABLE IF NOT EXISTS for all tables on startup.
    void initSchema();
};

#endif
