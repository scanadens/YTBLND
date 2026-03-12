#include "SqliteDataManager.h"
#include <iostream>

// ── Constructor / Destructor ──────────────────────────────────────────────────

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

// ── Private helpers ───────────────────────────────────────────────────────────

void SqliteDataManager::initSchema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "  user_id  TEXT PRIMARY KEY,"
        "  username TEXT NOT NULL,"
        "  email    TEXT,"
        "  password TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SqliteDataManager] Schema init failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

// ── DataManager interface ─────────────────────────────────────────────────────

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
