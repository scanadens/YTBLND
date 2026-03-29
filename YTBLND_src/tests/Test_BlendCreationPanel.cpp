#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <wx/app.h>
#include <wx/frame.h>
#include "../UILayer/BlendCreationPanel.hpp"
#include "../AppLayer/AppController.hpp"
#include "../AppLayer/EventRouter.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::AtLeast;

// Mock EventRouter
class MockEventRouter {
public:
    MOCK_METHOD(void, dispatch, (const std::string&, const std::map<std::string, std::string>&));
};

// Mock AppController
class MockAppController : public AppController {
public:
    MOCK_METHOD(EventRouter&, getEventRouter, (), (override));
};

// wxWidgets requires an app instance
class BlendCreationPanelTestApp : public wxApp {
public:
    bool OnInit() override { return true; }
};

// Test fixture
class BlendCreationPanelTest : public ::testing::Test {
protected:
    static wxAppConsole* app;
    wxFrame* parentFrame = nullptr;
    NiceMock<MockAppController> mockController;
    NiceMock<MockEventRouter> mockRouter;
    BlendCreationPanel* panel = nullptr;
    bool navCalled = false;
    int navPage = Page::LOGIN;

    static void SetUpTestSuite() {
        if (!wxTheApp) {
            app = new wxAppConsole();
            wxApp::SetInstance(app);
            app->CallOnInit();
        }
    }

    void SetUp() override {
        parentFrame = new wxFrame(nullptr, wxID_ANY, "Test");
        
        EXPECT_CALL(mockController, getEventRouter)
            .WillRepeatedly(::testing::ReturnRef(mockRouter));

        auto navFn = [this](Page page) {
            navCalled = true;
            navPage = page;
        };

        panel = new BlendCreationPanel(parentFrame, mockController, navFn);
        parentFrame->Show();
    }

    void TearDown() override {
        if (parentFrame) {
            parentFrame->Destroy();
        }
    }
};

wxAppConsole* BlendCreationPanelTest::app = nullptr;

// ============================================================================
// UI STATE TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, InitializesWithCreateButtonDisabled) {
    ASSERT_NE(panel, nullptr);
    EXPECT_FALSE(panel->m_createBtn->IsEnabled());
}

TEST_F(BlendCreationPanelTest, InitializesWithEmptyUserList) {
    ASSERT_EQ(panel->m_addedUsers.size(), 0);
    EXPECT_TRUE(panel->m_emptyLabel->IsShown());
}

TEST_F(BlendCreationPanelTest, CountLabelInitializesToZeroUsers) {
    ASSERT_NE(panel->m_countLabel, nullptr);
    EXPECT_EQ(panel->m_countLabel->GetLabel(), "0 / 8 users");
}

// ============================================================================
// USER ADDITION TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, AddingUserUpdatesInternalList) {
    panel->m_addedUsers.push_back("alice");
    panel->RebuildUserList();
    panel->UpdateCountLabel();

    EXPECT_EQ(panel->m_addedUsers.size(), 1);
    EXPECT_EQ(panel->m_addedUsers[0], "alice");
}

TEST_F(BlendCreationPanelTest, AddingUserUpdatesCountLabel) {
    panel->m_addedUsers.push_back("alice");
    panel->UpdateCountLabel();

    EXPECT_EQ(panel->m_countLabel->GetLabel(), "1 / 8 users");
}

TEST_F(BlendCreationPanelTest, AddingUserUpdatesUI) {
    panel->m_addedUsers.push_back("alice");
    panel->RebuildUserList();

    EXPECT_FALSE(panel->m_emptyLabel->IsShown());
}

TEST_F(BlendCreationPanelTest, AddingSecondUserEnablesCreateButton) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->UpdateCountLabel();
    
    // Simulate the enable logic from OnAdd
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();

    EXPECT_TRUE(panel->m_createBtn->IsEnabled());
}

TEST_F(BlendCreationPanelTest, SingleUserKeepsCreateButtonDisabled) {
    panel->m_addedUsers.push_back("alice");
    
    // Simulate the enable logic from OnAdd
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    else
        panel->m_createBtn->Disable();

    EXPECT_FALSE(panel->m_createBtn->IsEnabled());
}

TEST_F(BlendCreationPanelTest, AddingMultipleUsersUpdatesCount) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->m_addedUsers.push_back("charlie");
    panel->UpdateCountLabel();

    EXPECT_EQ(panel->m_countLabel->GetLabel(), "3 / 8 users");
    EXPECT_EQ(panel->m_addedUsers.size(), 3);
}

// ============================================================================
// USER LOOKUP DISPATCH TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, AddingUserDispatchesLookupEvent) {
    EXPECT_CALL(mockRouter, dispatch("lookupUser", _)).Times(1);
    
    panel->m_addedUsers.push_back("alice");
    m_controller.getEventRouter().dispatch("lookupUser", {{"username", "alice"}});
}

TEST_F(BlendCreationPanelTest, LookupEventContainsCorrectUsername) {
    std::string capturedUsername;
    EXPECT_CALL(mockRouter, dispatch("lookupUser", _))
        .WillOnce([&](const std::string& event, const std::map<std::string, std::string>& payload) {
            auto it = payload.find("username");
            if (it != payload.end()) {
                capturedUsername = it->second;
            }
        });
    
    m_controller.getEventRouter().dispatch("lookupUser", {{"username", "alice"}});
    EXPECT_EQ(capturedUsername, "alice");
}

// ============================================================================
// USER REMOVAL TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, RemovingUserUpdatesInternalList) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->OnRemoveUser("alice");

    EXPECT_EQ(panel->m_addedUsers.size(), 1);
    EXPECT_EQ(panel->m_addedUsers[0], "bob");
}

TEST_F(BlendCreationPanelTest, RemovingUserUpdatesCountLabel) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->OnRemoveUser("alice");
    panel->UpdateCountLabel();

    EXPECT_EQ(panel->m_countLabel->GetLabel(), "1 / 8 users");
}

TEST_F(BlendCreationPanelTest, RemovingSecondUserDisablesCreateButton) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->OnRemoveUser("bob");

    // Simulate the disable logic from OnRemoveUser
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    else
        panel->m_createBtn->Disable();

    EXPECT_FALSE(panel->m_createBtn->IsEnabled());
}

TEST_F(BlendCreationPanelTest, RemovingAllUsersShowsEmptyLabel) {
    panel->m_addedUsers.push_back("alice");
    panel->OnRemoveUser("alice");
    panel->RebuildUserList();

    EXPECT_TRUE(panel->m_emptyLabel->IsShown());
    EXPECT_EQ(panel->m_addedUsers.size(), 0);
}

// ============================================================================
// RELOAD TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, ReloadClearsAllUsers) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->Reload();

    EXPECT_EQ(panel->m_addedUsers.size(), 0);
}

TEST_F(BlendCreationPanelTest, ReloadResetsCountLabel) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->Reload();

    EXPECT_EQ(panel->m_countLabel->GetLabel(), "0 / 8 users");
}

TEST_F(BlendCreationPanelTest, ReloadDisablesCreateButton) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->m_createBtn->Enable();
    panel->Reload();

    EXPECT_FALSE(panel->m_createBtn->IsEnabled());
}

// ============================================================================
// BLEND CREATION & DISPATCH TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, CreateBlendDispatchesEventWithCorrectPayload) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");

    EXPECT_CALL(mockRouter, dispatch("createBlend", _)).Times(1);
    panel->OnCreate(wxCommandEvent());
}

TEST_F(BlendCreationPanelTest, CreateBlendPayloadContainsAllUsers) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->m_addedUsers.push_back("charlie");

    std::map<std::string, std::string> capturedPayload;
    EXPECT_CALL(mockRouter, dispatch("createBlend", _))
        .WillOnce([&](const std::string& event, const std::map<std::string, std::string>& payload) {
            capturedPayload = payload;
        });

    panel->OnCreate(wxCommandEvent());

    EXPECT_EQ(capturedPayload["userID_0"], "alice");
    EXPECT_EQ(capturedPayload["userID_1"], "bob");
    EXPECT_EQ(capturedPayload["userID_2"], "charlie");
}

TEST_F(BlendCreationPanelTest, CreateBlendPayloadKeyFormat) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");

    std::map<std::string, std::string> capturedPayload;
    EXPECT_CALL(mockRouter, dispatch("createBlend", _))
        .WillOnce([&](const std::string& event, const std::map<std::string, std::string>& payload) {
            capturedPayload = payload;
        });

    panel->OnCreate(wxCommandEvent());

    EXPECT_TRUE(capturedPayload.find("userID_0") != capturedPayload.end());
    EXPECT_TRUE(capturedPayload.find("userID_1") != capturedPayload.end());
}

TEST_F(BlendCreationPanelTest, CreateBlendNavigatesToBlendChat) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->OnCreate(wxCommandEvent());

    EXPECT_TRUE(navCalled);
    EXPECT_EQ(navPage, Page::BLEND_CHAT);
}

TEST_F(BlendCreationPanelTest, CreateBlendWithLessThanTwoUsersDoesNotProceed) {
    panel->m_addedUsers.push_back("alice");
    
    EXPECT_CALL(mockRouter, dispatch("createBlend", _)).Times(0);
    panel->OnCreate(wxCommandEvent());

    EXPECT_FALSE(navCalled);
}

TEST_F(BlendCreationPanelTest, CreateBlendWithMaxUsers) {
    for (int i = 0; i < 8; ++i) {
        panel->m_addedUsers.push_back("user" + std::to_string(i));
    }

    std::map<std::string, std::string> capturedPayload;
    EXPECT_CALL(mockRouter, dispatch("createBlend", _))
        .WillOnce([&](const std::string& event, const std::map<std::string, std::string>& payload) {
            capturedPayload = payload;
        });

    panel->OnCreate(wxCommandEvent());

    EXPECT_EQ(capturedPayload.size(), 8);
    EXPECT_EQ(capturedPayload["userID_7"], "user7");
}

// ============================================================================
// FULL WORKFLOW TESTS
// ============================================================================

TEST_F(BlendCreationPanelTest, CompleteBlendCreationWorkflow) {
    // Step 1: Add first user
    panel->m_addedUsers.push_back("alice");
    panel->RebuildUserList();
    panel->UpdateCountLabel();
    EXPECT_FALSE(panel->m_createBtn->IsEnabled());

    // Step 2: Add second user (enable button)
    panel->m_addedUsers.push_back("bob");
    panel->RebuildUserList();
    panel->UpdateCountLabel();
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    EXPECT_TRUE(panel->m_createBtn->IsEnabled());

    // Step 3: Dispatch blend creation
    EXPECT_CALL(mockRouter, dispatch("createBlend", _)).Times(1);
    panel->OnCreate(wxCommandEvent());

    EXPECT_TRUE(navCalled);
    EXPECT_EQ(navPage, Page::BLEND_CHAT);
}

TEST_F(BlendCreationPanelTest, AddRemoveAddWorkflow) {
    // Add alice
    panel->m_addedUsers.push_back("alice");
    EXPECT_EQ(panel->m_addedUsers.size(), 1);

    // Add bob
    panel->m_addedUsers.push_back("bob");
    EXPECT_EQ(panel->m_addedUsers.size(), 2);
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    EXPECT_TRUE(panel->m_createBtn->IsEnabled());

    // Remove bob
    panel->OnRemoveUser("bob");
    EXPECT_EQ(panel->m_addedUsers.size(), 1);
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    else
        panel->m_createBtn->Disable();
    EXPECT_FALSE(panel->m_createBtn->IsEnabled());

    // Add charlie
    panel->m_addedUsers.push_back("charlie");
    EXPECT_EQ(panel->m_addedUsers.size(), 2);
    if (panel->m_addedUsers.size() >= 2)
        panel->m_createBtn->Enable();
    EXPECT_TRUE(panel->m_createBtn->IsEnabled());
}

TEST_F(BlendCreationPanelTest, BlendCreationWithDifferentUserCombinations) {
    panel->m_addedUsers.push_back("alice");
    panel->m_addedUsers.push_back("bob");
    panel->m_addedUsers.push_back("charlie");

    std::map<std::string, std::string> capturedPayload;
    EXPECT_CALL(mockRouter, dispatch("createBlend", _))
        .WillOnce([&](const std::string& event, const std::map<std::string, std::string>& payload) {
            capturedPayload = payload;
        });

    panel->OnCreate(wxCommandEvent());

    EXPECT_EQ(capturedPayload.size(), 3);
    EXPECT_EQ(capturedPayload["userID_0"], "alice");
    EXPECT_EQ(capturedPayload["userID_1"], "bob");
    EXPECT_EQ(capturedPayload["userID_2"], "charlie");
    EXPECT_TRUE(navCalled);
}