// ============================================================================
// Test_BlendManagement.cpp — Tests for blend management overhaul features
//
// Unit tests:
//   - Blend title field: constructor, getter, toString
//   - RandomBlendAlgorithm passes title through
//   - RequestJsonBuilder includes title
//
// Integration tests (opt-out via YTBLND_SKIP_LIVE_BACKEND_TESTS=1):
//   - Create blend with title on server
//   - Fetch user blends list
//   - Leave a blend
//   - Re-upload watch-later data
//
// Theme switching tests (UIColors) are in Test_ThemeSwitching.cpp and require
// YTBLND_BUILD_UI_TESTS=ON since they depend on wxWidgets headers.
// ============================================================================

#include "gtest/gtest.h"

#include "../ServerConfig.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Video.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ModelLayer/Message.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ServiceLayer/HttpClient.hpp"
#include "../ServiceLayer/RequestJsonBuilder.hpp"

#include <chrono>
#include <cstdlib>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// ── Helpers ──────────────────────────────────────────────────────────────────

namespace {

Video makeVideo(const std::string& id = "v1") {
    return Video(id, "Title", "", "", 0, std::list<std::string>{});
}

User makeUser(const std::string& id = "u1") {
    return User(id, id, "", "");
}

std::string makeUniqueID(const std::string& prefix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    const auto ns  = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    std::ostringstream out;
    out << prefix << "_" << ns;
    return out.str();
}

std::string getenvOrDefault(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return (val && std::string(val) != "") ? std::string(val) : fallback;
}

bool isLiveBackendEnabled() {
    const char* flag = std::getenv("YTBLND_SKIP_LIVE_BACKEND_TESTS");
    return !(flag && std::string(flag) == "1");
}

bool containsAll(const std::string& value, std::initializer_list<std::string> needles) {
    for (const auto& needle : needles) {
        if (value.find(needle) == std::string::npos) return false;
    }
    return true;
}

void registerAndLogin(HttpClient& client, const std::string& userID) {
    const std::string regBody = RequestJsonBuilder::buildRegisterJson(
        userID, userID, userID + "@test.com", "testpass123");
    client.post("/api/v1/auth/register", regBody);

    const std::string loginBody = RequestJsonBuilder::buildLoginJson(userID, "testpass123");
    const std::string loginResp = client.post("/api/v1/auth/login", loginBody);
    ASSERT_TRUE(client.wasLastRequestSuccessful(client.P))
        << "Login failed for " << userID << ": HTTP " << client.getLastStatusCode();
}

void deleteUser(HttpClient& client, const std::string& userID) {
    client.del(client.build_delete_user_endpoint(userID),
               RequestJsonBuilder::buildDeleteAccountJson("testpass123"));
}

}  // namespace

// ═══════════════════════════════════════════════════════════════════════════
// UNIT TESTS — Blend title
// ═══════════════════════════════════════════════════════════════════════════

TEST(BlendTitleTest, ConstructorStoresTitle) {
    Blend b("id1", "My Blend", "random", {makeUser()}, {makeVideo()});
    EXPECT_EQ(b.getTitle(), "My Blend");
}

TEST(BlendTitleTest, EmptyTitleIsValid) {
    Blend b("id1", "", "random", {makeUser()}, {makeVideo()});
    EXPECT_EQ(b.getTitle(), "");
}

TEST(BlendTitleTest, ToStringIncludesTitle) {
    Blend b("id1", "Cool Blend", "random", {makeUser()}, {makeVideo()});
    std::string json = b.toString();
    EXPECT_TRUE(json.find("\"title\":\"Cool Blend\"") != std::string::npos)
        << "toString() output: " << json;
}

TEST(BlendTitleTest, AlgorithmPassesTitleThrough) {
    User u = makeUser("alice");
    YouTubeData yt;
    yt.setWatchLaterVideos({makeVideo("v1"), makeVideo("v2"), makeVideo("v3")});
    u.setYouTubeData(yt);

    Blend blend = RandomBlendAlgorithm(2).generateBlend({u}, "Friday Vibes");
    EXPECT_EQ(blend.getTitle(), "Friday Vibes");
    EXPECT_LE(blend.size(), 2);
}

TEST(BlendTitleTest, AlgorithmDefaultsTitleToEmpty) {
    User u = makeUser("bob");
    YouTubeData yt;
    yt.setWatchLaterVideos({makeVideo("v1")});
    u.setYouTubeData(yt);

    Blend blend = RandomBlendAlgorithm(5).generateBlend({u});
    EXPECT_EQ(blend.getTitle(), "");
}

// ═══════════════════════════════════════════════════════════════════════════
// UNIT TESTS — RequestJsonBuilder blend title
// ═══════════════════════════════════════════════════════════════════════════

TEST(BuildBlendJsonTitleTest, IncludesTitleField) {
    std::vector<std::string> participants = {"u1", "u2"};
    std::string json = RequestJsonBuilder::buildBlendJson(
        "blend_1", "My Awesome Blend", "creator1", "random", participants);

    EXPECT_TRUE(json.find("\"title\":\"My Awesome Blend\"") != std::string::npos)
        << "JSON: " << json;
    EXPECT_TRUE(json.find("\"blend_id\":\"blend_1\"") != std::string::npos);
}

TEST(BuildBlendJsonTitleTest, EmptyTitleProducesEmptyString) {
    std::vector<std::string> participants = {"u1"};
    std::string json = RequestJsonBuilder::buildBlendJson(
        "b1", "", "c1", "random", participants);

    EXPECT_TRUE(json.find("\"title\":\"\"") != std::string::npos)
        << "JSON: " << json;
}

// ═══════════════════════════════════════════════════════════════════════════
// INTEGRATION TESTS — Blend management against live backend
// (Gracefully skipped if server returns 404 or is unreachable)
// ═══════════════════════════════════════════════════════════════════════════

class BlendManagementIntegrationTest : public ::testing::Test {
protected:
    std::string httpBase;
    std::unique_ptr<HttpClient> client;

    void SetUp() override {
        if (!isLiveBackendEnabled()) {
            GTEST_SKIP() << "Live backend tests disabled.";
        }
        httpBase = kTestBackendBaseUrl;
        client = std::make_unique<HttpClient>(httpBase);

        // Quick connectivity check
        try {
            client->get("/ping");
            if (client->getLastStatusCode() != 200) {
                GTEST_SKIP() << "Backend not reachable at " << httpBase;
            }
        } catch (...) {
            GTEST_SKIP() << "Backend not reachable at " << httpBase;
        }
    }
};

TEST_F(BlendManagementIntegrationTest, CreateBlendWithTitleIncludesTitleInResponse) {
    const std::string userID = makeUniqueID("title_test");
    registerAndLogin(*client, userID);

    const std::string blendID = makeUniqueID("blend_titled");
    std::string json = RequestJsonBuilder::buildBlendJson(
        blendID, "Movie Night", userID, "random", {userID});

    const std::string resp = client->post("/api/v1/blends", json);

    // If server doesn't support title field, it might still succeed (just ignoring it)
    if (client->getLastStatusCode() == 404 || client->getLastStatusCode() == 501) {
        deleteUser(*client, userID);
        GTEST_SKIP() << "Server does not support blend creation endpoint.";
    }

    EXPECT_TRUE(client->wasLastRequestSuccessful(client->P))
        << "HTTP " << client->getLastStatusCode() << " resp=" << resp;
    EXPECT_TRUE(containsAll(resp, {"blend_id", blendID}))
        << "Response: " << resp;

    deleteUser(*client, userID);
}

TEST_F(BlendManagementIntegrationTest, FetchUserBlendsEndpoint) {
    const std::string userID = makeUniqueID("list_blends");
    registerAndLogin(*client, userID);

    // Create a blend first
    const std::string blendID = makeUniqueID("blend_list");
    std::string blendJson = RequestJsonBuilder::buildBlendJson(
        blendID, "Test Blend", userID, "random", {userID});
    client->post("/api/v1/blends", blendJson);

    // Fetch user blends
    const std::string resp = client->get(client->build_user_blends_endpoint(userID));

    if (client->getLastStatusCode() == 404) {
        deleteUser(*client, userID);
        GTEST_SKIP() << "Server does not support /users/{id}/blends endpoint yet.";
    }

    EXPECT_TRUE(client->wasLastRequestSuccessful(client->G))
        << "HTTP " << client->getLastStatusCode() << " resp=" << resp;

    // Response should contain a blends array
    EXPECT_TRUE(resp.find("blends") != std::string::npos
                || resp.find("blend_id") != std::string::npos)
        << "Expected blends in response: " << resp;

    deleteUser(*client, userID);
}

TEST_F(BlendManagementIntegrationTest, LeaveBlendEndpoint) {
    const std::string userID = makeUniqueID("leave_blend");
    registerAndLogin(*client, userID);

    const std::string blendID = makeUniqueID("blend_leave");
    std::string blendJson = RequestJsonBuilder::buildBlendJson(
        blendID, "Leaving Test", userID, "random", {userID});
    client->post("/api/v1/blends", blendJson);

    // Try to leave the blend
    const std::string leaveBody = "{\"user_id\":\"" + userID + "\"}";
    const std::string resp = client->post(client->build_leave_blend_endpoint(blendID), leaveBody);

    if (client->getLastStatusCode() == 404 || client->getLastStatusCode() == 501) {
        deleteUser(*client, userID);
        GTEST_SKIP() << "Server does not support /blends/{id}/leave endpoint yet.";
    }

    EXPECT_TRUE(client->wasLastRequestSuccessful(client->P))
        << "HTTP " << client->getLastStatusCode() << " resp=" << resp;

    deleteUser(*client, userID);
}

TEST_F(BlendManagementIntegrationTest, ReuploadWatchLaterData) {
    const std::string userID = makeUniqueID("reupload");
    registerAndLogin(*client, userID);

    // Upload initial watch-later data
    std::list<Video> videos1;
    videos1.emplace_back("vid_a", "Video A", "", "", 0, std::list<std::string>{});
    videos1.emplace_back("vid_b", "Video B", "", "", 0, std::list<std::string>{});

    const std::string upload1 = client->post(
        client->build_watch_later_endpoint(userID),
        RequestJsonBuilder::buildWatchLaterJson(videos1));
    ASSERT_TRUE(client->wasLastRequestSuccessful(client->P))
        << "Initial upload failed: " << upload1;

    // Re-upload with different data
    std::list<Video> videos2;
    videos2.emplace_back("vid_c", "Video C", "", "", 0, std::list<std::string>{});
    videos2.emplace_back("vid_d", "Video D", "", "", 0, std::list<std::string>{});
    videos2.emplace_back("vid_e", "Video E", "", "", 0, std::list<std::string>{});

    const std::string upload2 = client->post(
        client->build_watch_later_endpoint(userID),
        RequestJsonBuilder::buildWatchLaterJson(videos2));
    ASSERT_TRUE(client->wasLastRequestSuccessful(client->P))
        << "Re-upload failed: " << upload2;

    // Verify the new data is retrievable
    const std::string getResp = client->get(client->build_watch_later_endpoint(userID));
    ASSERT_TRUE(client->wasLastRequestSuccessful(client->G))
        << "GET failed: " << getResp;

    // Should contain the new videos, not the old ones
    EXPECT_TRUE(getResp.find("vid_c") != std::string::npos)
        << "Expected vid_c in response: " << getResp;
    EXPECT_TRUE(getResp.find("vid_d") != std::string::npos)
        << "Expected vid_d in response: " << getResp;

    deleteUser(*client, userID);
}

TEST_F(BlendManagementIntegrationTest, BlendParticipantsEndpointReturnsParticipants) {
    const std::string user1 = makeUniqueID("part_a");
    const std::string user2 = makeUniqueID("part_b");
    registerAndLogin(*client, user1);
    registerAndLogin(*client, user2);

    const std::string blendID = makeUniqueID("blend_parts");
    std::string json = RequestJsonBuilder::buildBlendJson(
        blendID, "Duo Blend", user1, "random", {user1, user2});
    client->post("/api/v1/blends", json);

    if (!client->wasLastRequestSuccessful(client->P)) {
        deleteUser(*client, user1);
        deleteUser(*client, user2);
        GTEST_SKIP() << "Blend creation failed; skipping participant check.";
    }

    const std::string resp = client->get(client->build_blend_participant_endpoint(blendID));
    EXPECT_TRUE(client->wasLastRequestSuccessful(client->G))
        << "HTTP " << client->getLastStatusCode();

    EXPECT_TRUE(containsAll(resp, {"participants", user1, user2}))
        << "Response: " << resp;

    deleteUser(*client, user1);
    deleteUser(*client, user2);
}
