#include "gtest/gtest.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include "../ServiceLayer/SqliteDataManager.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/AppState.hpp"

// ── Helpers ───────────────────────────────────────────────────────────────────

// Every test that uses SqliteDataManager gets a fresh in-memory DB.
static SqliteDataManager makeDB() {
    return SqliteDataManager(":memory:");
}

static User makeUser(const std::string& id = "u1",
                     const std::string& username = "alice",
                     const std::string& email    = "alice@example.com",
                     const std::string& password = "secret") {
    return User(id, username, email, password);
}

static std::string uniqueUserId(const std::string& prefix) {
    static std::atomic<unsigned long long> counter{0};
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    const auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    // Time + counter avoids collisions both within and across test runs.
    return prefix + "_" + std::to_string(ticks) + "_" + std::to_string(++counter);
}

static std::string uniqueDbPath(const std::string& prefix) {
    const std::filesystem::path base = std::filesystem::temp_directory_path();
    return (base / (uniqueUserId(prefix) + ".db")).string();
}

static bool canReachBackend() {
    HttpClient http("http://137.220.58.22:8080");
    try {
        const std::string resp = http.get("/ping");
        return http.wasLastRequestSuccessful(http.G) &&
               resp.find("pong") != std::string::npos;
    } catch (...) {
        return false;
    }
}

static constexpr const char* kDummyPassword = "123456";

static bool loginWithCredentials(AppController& ctrl,
                                 const std::string& userID,
                                 const std::string& password) {
    if (AppState::getInstance().getCurrentUser()) {
        delete AppState::getInstance().getCurrentUser();
        AppState::getInstance().setCurrentUser(nullptr);
    }

    ctrl.getEventRouter().dispatch("login", {
        {"userID", userID},
        {"password", password}
    });

    return AppState::getInstance().getCurrentUser() != nullptr;
}

static void ensureDummyAccountExists(AppController& ctrl,
                                     const std::string& userID,
                                     const std::string& username) {
    // Prefer existing seeded dummy accounts. If absent, create them once.
    if (loginWithCredentials(ctrl, userID, kDummyPassword)) {
        return;
    }

    ctrl.getEventRouter().dispatch("register", {
        {"userID", userID},
        {"username", username},
        {"email", userID + "@ytblnd.test"},
        {"password", kDummyPassword}
    });

    if (AppState::getInstance().getCurrentUser()) {
        delete AppState::getInstance().getCurrentUser();
        AppState::getInstance().setCurrentUser(nullptr);
    }

    ASSERT_TRUE(loginWithCredentials(ctrl, userID, kDummyPassword))
        << "Could not login with dummy account userID=" << userID;
}

// ── SqliteDataManager tests ───────────────────────────────────────────────────

TEST(SqliteDataManagerTest, CreateUser_Succeeds) {
    SqliteDataManager db = makeDB();
    EXPECT_TRUE(db.createUser(makeUser()));
}

TEST(SqliteDataManagerTest, CreateUser_DuplicateID_Fails) {
    SqliteDataManager db = makeDB();
    db.createUser(makeUser());
    EXPECT_FALSE(db.createUser(makeUser())); // same userID → PRIMARY KEY conflict
}

TEST(SqliteDataManagerTest, FindUserByID_ReturnsUser) {
    SqliteDataManager db = makeDB();
    db.createUser(makeUser("u1", "alice", "a@b.com", "pw"));

    std::optional<User> result = db.findUserByID("u1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("u1",    result->getUserID());
    EXPECT_EQ("alice", result->getUsername());
    EXPECT_EQ("a@b.com", result->getEmail());
    EXPECT_EQ("pw",    result->getPassword());
}

TEST(SqliteDataManagerTest, FindUserByID_NotFound_ReturnsNullopt) {
    SqliteDataManager db = makeDB();
    EXPECT_FALSE(db.findUserByID("nonexistent").has_value());
}

TEST(SqliteDataManagerTest, ValidatePassword_CorrectPassword_ReturnsTrue) {
    SqliteDataManager db = makeDB();
    db.createUser(makeUser("u1", "alice", "", "correct"));
    EXPECT_TRUE(db.validatePassword("u1", "correct"));
}

TEST(SqliteDataManagerTest, ValidatePassword_WrongPassword_ReturnsFalse) {
    SqliteDataManager db = makeDB();
    db.createUser(makeUser("u1", "alice", "", "correct"));
    EXPECT_FALSE(db.validatePassword("u1", "wrong"));
}

TEST(SqliteDataManagerTest, ValidatePassword_UnknownUser_ReturnsFalse) {
    SqliteDataManager db = makeDB();
    EXPECT_FALSE(db.validatePassword("nobody", "anything"));
}

// ── AppController auth flow tests ─────────────────────────────────────────────
// These use the real AppController (which opens "ytblnd.db").
// We reset AppState between tests to avoid cross-test pollution.

class AppControllerAuthTest : public ::testing::Test {
protected:
    std::string dbPath;

    void SetUp() override {
        // Each test gets a private SQLite file so repeated runs and parallel
        // runs never reuse rows from previous executions.
        dbPath = uniqueDbPath("ytblnd_auth_test");

        // Clear any leftover session state from previous tests
        AppState& state = AppState::getInstance();
        if (state.getCurrentUser()) {
            delete state.getCurrentUser();
            state.setCurrentUser(nullptr);
        }
        state.clearSession();
    }

    void TearDown() override {
        AppState& state = AppState::getInstance();
        if (state.getCurrentUser()) {
            delete state.getCurrentUser();
            state.setCurrentUser(nullptr);
        }
        state.clearSession();

        // Best-effort cleanup for the per-test DB file.
        std::error_code ec;
        std::filesystem::remove(dbPath, ec);
    }
};

TEST_F(AppControllerAuthTest, Register_SetsCurrentUser) {
    // These AppController tests exercise the live server-backed auth flow.
    if (!canReachBackend()) {
        GTEST_SKIP() << "Skipping live AppController auth test: backend unavailable";
    }

    AppController ctrl(dbPath);
    const std::string userID = "2";

    // Use the agreed dummy account, creating it once if this environment is fresh.
    ensureDummyAccountExists(ctrl, userID, "dummy-two");

    // Successful register/login flow must leave AppState with the expected user.
    ASSERT_NE(nullptr, AppState::getInstance().getCurrentUser());
    EXPECT_EQ(userID, AppState::getInstance().getCurrentUser()->getUserID());
}

TEST_F(AppControllerAuthTest, Login_ValidCredentials_SetsCurrentUser) {
    // Skip rather than fail when the backend process is intentionally offline.
    if (!canReachBackend()) {
        GTEST_SKIP() << "Skipping live AppController auth test: backend unavailable";
    }

    AppController ctrl(dbPath);
    const std::string userID = "1";

    // Ensure the seeded dummy account exists before validating login behavior.
    ensureDummyAccountExists(ctrl, userID, "dummy-one");

    // Login success contract: current user pointer is populated with same ID.
    ASSERT_NE(nullptr, AppState::getInstance().getCurrentUser());
    EXPECT_EQ(userID,
              AppState::getInstance().getCurrentUser()->getUserID());
}

TEST_F(AppControllerAuthTest, Login_WrongPassword_DoesNotSetUser) {
    // Negative case verifies that bad credentials do not mutate session state.
    if (!canReachBackend()) {
        GTEST_SKIP() << "Skipping live AppController auth test: backend unavailable";
    }

    AppController ctrl(dbPath);
    const std::string userID = "1";
    ensureDummyAccountExists(ctrl, userID, "dummy-one");

    // Attempt login with known-wrong password against an existing user.
    const bool loggedIn = loginWithCredentials(ctrl, userID, "wrong");
    EXPECT_FALSE(loggedIn);
    EXPECT_EQ(nullptr, AppState::getInstance().getCurrentUser());
}

TEST_F(AppControllerAuthTest, Logout_ClearsCurrentUser) {
    // Logout should clear AppState after an authenticated session.
    if (!canReachBackend()) {
        GTEST_SKIP() << "Skipping live AppController auth test: backend unavailable";
    }

    AppController ctrl(dbPath);
    const std::string userID = "2";
    ensureDummyAccountExists(ctrl, userID, "dummy-two");
    ASSERT_NE(nullptr, AppState::getInstance().getCurrentUser());

    // EventRouter path mirrors what the UI triggers during a real logout action.
    ctrl.getEventRouter().dispatch("logout", {});
    EXPECT_EQ(nullptr, AppState::getInstance().getCurrentUser());
}
