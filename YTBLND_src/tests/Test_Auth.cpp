#include "gtest/gtest.h"
#include "../ServiceLayer/SqliteDataManager.h"
#include "../AppLayer/AppController.h"
#include "../AppLayer/AppState.h"

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
    void SetUp() override {
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
    }
};

TEST_F(AppControllerAuthTest, Register_SetsCurrentUser) {
    AppController ctrl;
    ctrl.getEventRouter().dispatch("register", {
        {"userID",   "test_reg_user"},
        {"username", "Tester"},
        {"email",    "test@example.com"},
        {"password", "pass123"}
    });
    EXPECT_NE(nullptr, AppState::getInstance().getCurrentUser());
    EXPECT_EQ("test_reg_user", AppState::getInstance().getCurrentUser()->getUserID());
}

TEST_F(AppControllerAuthTest, Login_ValidCredentials_SetsCurrentUser) {
    // Register first so the account exists
    AppController ctrl;
    ctrl.getEventRouter().dispatch("register", {
        {"userID",   "test_login_user"},
        {"username", "LoginTester"},
        {"email",    ""},
        {"password", "mypassword"}
    });

    // Clear session to simulate logging out then back in
    if (AppState::getInstance().getCurrentUser()) {
        delete AppState::getInstance().getCurrentUser();
        AppState::getInstance().setCurrentUser(nullptr);
    }

    ctrl.getEventRouter().dispatch("login", {
        {"userID",   "test_login_user"},
        {"password", "mypassword"}
    });

    ASSERT_NE(nullptr, AppState::getInstance().getCurrentUser());
    EXPECT_EQ("test_login_user",
              AppState::getInstance().getCurrentUser()->getUserID());
}

TEST_F(AppControllerAuthTest, Login_WrongPassword_DoesNotSetUser) {
    AppController ctrl;
    ctrl.getEventRouter().dispatch("register", {
        {"userID", "test_pw_user"}, {"username", "PWUser"},
        {"email", ""}, {"password", "right"}
    });
    if (AppState::getInstance().getCurrentUser()) {
        delete AppState::getInstance().getCurrentUser();
        AppState::getInstance().setCurrentUser(nullptr);
    }

    ctrl.getEventRouter().dispatch("login", {
        {"userID", "test_pw_user"}, {"password", "wrong"}
    });
    EXPECT_EQ(nullptr, AppState::getInstance().getCurrentUser());
}

TEST_F(AppControllerAuthTest, Logout_ClearsCurrentUser) {
    AppController ctrl;
    ctrl.getEventRouter().dispatch("register", {
        {"userID", "test_logout_user"}, {"username", "LogoutUser"},
        {"email", ""}, {"password", "pw"}
    });
    ASSERT_NE(nullptr, AppState::getInstance().getCurrentUser());

    ctrl.getEventRouter().dispatch("logout", {});
    EXPECT_EQ(nullptr, AppState::getInstance().getCurrentUser());
}
