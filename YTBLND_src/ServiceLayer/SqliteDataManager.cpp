#include "SqliteDataManager.hpp"
#include <iostream>
#include <ctime>

// -- Constructor / Destructor --------------------------------------------------

SqliteDataManager::SqliteDataManager(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] Failed to open DB: "
                  << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        db = nullptr;
        return;
    }
    initSchema();
}

SqliteDataManager::~SqliteDataManager() {
    if (db) sqlite3_close(db);
}

// -- Private helpers -----------------------------------------------------------

void SqliteDataManager::initSchema() {
    const char* sql =
        // -- Users -------------------------------------------------------------
        "CREATE TABLE IF NOT EXISTS users ("
        "  user_id  TEXT PRIMARY KEY,"
        "  username TEXT NOT NULL,"
        "  email    TEXT,"
        "  password TEXT NOT NULL"
        ");"

        // -- Watch Later videos per user ----------------------------------------
        // Stores each parsed + API-enriched video as a row; replaced in bulk
        // on re-upload. position preserves original playlist order for display.
        "CREATE TABLE IF NOT EXISTS user_watch_later ("
        "  user_id         TEXT NOT NULL,"
        "  video_id        TEXT NOT NULL,"
        "  title           TEXT,"
        "  channel_id      TEXT,"
        "  channel_name    TEXT,"
        "  channel_logo_url TEXT,"
        "  thumbnail_url   TEXT,"
        "  position        INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (user_id, video_id)"
        ");"

        // -- Blend metadata -----------------------------------------------------
        // creator_id kept separate for future delete/manage privileges.
        "CREATE TABLE IF NOT EXISTS blends ("
        "  blend_id   TEXT PRIMARY KEY,"
        "  creator_id TEXT NOT NULL,"
        "  algorithm  TEXT NOT NULL,"
        "  created_at TEXT NOT NULL"
        ");"

        // -- Blend participants -------------------------------------------------
        // One row per (blend, user) pair — enables fast lookup in both directions.
        "CREATE TABLE IF NOT EXISTS blend_participants ("
        "  blend_id TEXT NOT NULL,"
        "  user_id  TEXT NOT NULL,"
        "  PRIMARY KEY (blend_id, user_id)"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] Schema init failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }

    // Migration: add new columns to pre-existing databases that have the old
    // user_watch_later schema (SQLite ignores "duplicate column" errors).
    const char* migrations[] = {
        "ALTER TABLE user_watch_later ADD COLUMN channel_id       TEXT;",
        "ALTER TABLE user_watch_later ADD COLUMN channel_name     TEXT;",
        "ALTER TABLE user_watch_later ADD COLUMN channel_logo_url TEXT;",
        "ALTER TABLE user_watch_later ADD COLUMN thumbnail_url    TEXT;",
    };
    for (const char* m : migrations) {
        sqlite3_exec(db, m, nullptr, nullptr, nullptr); // ignore error if column exists
    }
}

// -- DataManager interface -----------------------------------------------------

bool SqliteDataManager::createUser(const User& user) {
    if (!db) return false;

    const char* sql =
        "INSERT INTO users (user_id, username, email, password) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] createUser prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.getUserID().c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getUsername().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getEmail().c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.getPassword().c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        // SQLITE_CONSTRAINT means the PRIMARY KEY (user_id) already exists
        std::cerr << "[SqliteDataManager] createUser failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }
    return true;
}

std::optional<User> SqliteDataManager::findUserByID(const std::string& userID) {
    if (!db) return std::nullopt;

    const char* sql =
        "SELECT user_id, username, email, password "
        "FROM users WHERE user_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] findUserByID prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string id       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string uname    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string email    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        result = User(id, uname, email, password);
    }

    sqlite3_finalize(stmt);
    return result;
}

bool SqliteDataManager::validatePassword(const std::string& userID,
                                         const std::string& password) {
    std::optional<User> user = findUserByID(userID);
    if (!user.has_value()) return false;
    return user->getPassword() == password;
}

// -- Watch Later ---------------------------------------------------------------

bool SqliteDataManager::saveWatchLater(const std::string& userID,
                                       const std::list<Video>& videos) {
    if (!db) return false;

    // Delete existing rows for this user then re-insert — handles re-uploads cleanly.
    const char* delSql = "DELETE FROM user_watch_later WHERE user_id = ?;";
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(db, delSql, -1, &del, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] saveWatchLater delete prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(del, 1, userID.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(del);
    sqlite3_finalize(del);

    const char* insSql =
        "INSERT OR REPLACE INTO user_watch_later "
        "(user_id, video_id, title, channel_id, channel_name, channel_logo_url, thumbnail_url, position) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(db, insSql, -1, &ins, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] saveWatchLater insert prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }

    int pos = 0;
    for (const Video& v : videos) {
        sqlite3_bind_text(ins, 1, userID.c_str(),                -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 2, v.getVideoID().c_str(),         -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 3, v.getTitle().c_str(),           -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 4, v.getChannelID().c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 5, v.getChannelName().c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 6, v.getChannelLogoURL().c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 7, v.getThumbnailURL().c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (ins, 8, pos++);
        if (sqlite3_step(ins) != SQLITE_DONE) {
            std::cerr << "[SqliteDataManager] saveWatchLater insert failed: "
                      << sqlite3_errmsg(db) << "\n";
        }
        sqlite3_reset(ins);
    }
    sqlite3_finalize(ins);
    return true;
}

std::list<Video> SqliteDataManager::loadWatchLater(const std::string& userID) {
    std::list<Video> result;
    if (!db) return result;

    const char* sql =
        "SELECT video_id, title, channel_id, channel_name, channel_logo_url, thumbnail_url "
        "FROM user_watch_later "
        "WHERE user_id = ? ORDER BY position ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] loadWatchLater prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return result;
    }
    sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);

    auto colText = [&](int col) -> std::string {
        const unsigned char* p = sqlite3_column_text(stmt, col);
        return p ? reinterpret_cast<const char*>(p) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result.emplace_back(
            colText(0), // videoID
            colText(1), // title
            colText(2), // channelID
            colText(5), // thumbnailURL
            0,
            std::list<std::string>{},
            colText(3), // channelName
            colText(4)  // channelLogoURL
        );
    }
    sqlite3_finalize(stmt);
    return result;
}

// -- Blend persistence ---------------------------------------------------------

bool SqliteDataManager::saveBlend(const std::string& blendID,
                                  const std::string& creatorID,
                                  const std::string& algorithm,
                                  const std::vector<std::string>& participantIDs) {
    if (!db) return false;

    // ISO timestamp for created_at
    std::time_t now = std::time(nullptr);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&now));

    // Upsert blend row
    const char* blendSql =
        "INSERT OR REPLACE INTO blends (blend_id, creator_id, algorithm, created_at) "
        "VALUES (?, ?, ?, ?);";
    sqlite3_stmt* bs = nullptr;
    if (sqlite3_prepare_v2(db, blendSql, -1, &bs, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] saveBlend blend prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }
    sqlite3_bind_text(bs, 1, blendID.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(bs, 2, creatorID.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(bs, 3, algorithm.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(bs, 4, timeBuf,            -1, SQLITE_TRANSIENT);
    sqlite3_step(bs);
    sqlite3_finalize(bs);

    // Replace participant rows — delete first so removed users are cleaned up
    const char* delSql = "DELETE FROM blend_participants WHERE blend_id = ?;";
    sqlite3_stmt* del = nullptr;
    sqlite3_prepare_v2(db, delSql, -1, &del, nullptr);
    sqlite3_bind_text(del, 1, blendID.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(del);
    sqlite3_finalize(del);

    const char* partSql =
        "INSERT OR IGNORE INTO blend_participants (blend_id, user_id) VALUES (?, ?);";
    sqlite3_stmt* ps = nullptr;
    if (sqlite3_prepare_v2(db, partSql, -1, &ps, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] saveBlend participants prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return false;
    }
    for (const auto& uid : participantIDs) {
        sqlite3_bind_text(ps, 1, blendID.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ps, 2, uid.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_step(ps);
        sqlite3_reset(ps);
    }
    sqlite3_finalize(ps);
    return true;
}

std::optional<std::string> SqliteDataManager::findBlendForUser(const std::string& userID) {
    if (!db) return std::nullopt;

    const char* sql =
        "SELECT bp.blend_id FROM blend_participants bp "
        "INNER JOIN blends b ON bp.blend_id = b.blend_id "
        "WHERE bp.user_id = ? "
        "ORDER BY b.created_at DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] findBlendForUser prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<std::string> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

std::vector<std::string> SqliteDataManager::loadBlendParticipants(const std::string& blendID) {
    std::vector<std::string> result;
    if (!db) return result;

    const char* sql =
        "SELECT user_id FROM blend_participants WHERE blend_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] loadBlendParticipants prepare failed: "
                  << sqlite3_errmsg(db) << "\n";
        return result;
    }
    sqlite3_bind_text(stmt, 1, blendID.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW)
        result.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    sqlite3_finalize(stmt);
    return result;
}
