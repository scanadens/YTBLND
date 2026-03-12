#include "gtest/gtest.h"

#include "../AppInfrastructure/WatchLaterParser.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.h"
#include "../ModelLayer/User.h"
#include "../ModelLayer/YouTubeData.h"
#include "../ModelLayer/Blend.h"

#include <list>
#include <string>

// Synthetic fixture — does not contain personal data
static const std::string WATCH_LATER_CSV =
    YTBLND_SRC_DIR "/tests/fixtures/Watch later-videos.csv";

// ── WatchLaterParser ──────────────────────────────────────────────────────────

TEST(WatchLaterParserTest, ParsesNonEmptyList) {
    std::list<Video> videos = WatchLaterParser(WATCH_LATER_CSV).parse();
    EXPECT_FALSE(videos.empty()) << "Expected at least one video in Watch Later CSV";
}

TEST(WatchLaterParserTest, EachVideoHasNonEmptyID) {
    std::list<Video> videos = WatchLaterParser(WATCH_LATER_CSV).parse();
    for (const Video& v : videos) {
        EXPECT_FALSE(v.getVideoID().empty()) << "Found a Video with an empty videoID";
    }
}

TEST(WatchLaterParserTest, NonIDFieldsAreEmpty) {
    std::list<Video> videos = WatchLaterParser(WATCH_LATER_CSV).parse();
    ASSERT_FALSE(videos.empty());
    const Video& first = videos.front();
    EXPECT_TRUE(first.getTitle().empty());
    EXPECT_TRUE(first.getChannelID().empty());
    EXPECT_EQ(0, first.getDuration());
}

// ── RandomBlendAlgorithm ──────────────────────────────────────────────────────

static User makeUserWithWatchLater(const std::string& userID, const std::string& csvPath) {
    std::list<Video> videos = WatchLaterParser(csvPath).parse();
    User user(userID, "", "", "");
    YouTubeData yd;
    yd.setWatchLaterVideos(videos);
    user.setYouTubeData(yd);
    return user;
}

TEST(RandomBlendAlgorithmTest, BlendHasAtMostVideosPerUserVideos) {
    User user = makeUserWithWatchLater("user1", WATCH_LATER_CSV);
    std::list<User> participants = {user};

    Blend blend = RandomBlendAlgorithm(3).generateBlend(participants);
    EXPECT_LE(blend.size(), 3);
}

TEST(RandomBlendAlgorithmTest, BlendVideoIDsAreNonEmpty) {
    User user = makeUserWithWatchLater("user1", WATCH_LATER_CSV);
    std::list<User> participants = {user};

    Blend blend = RandomBlendAlgorithm(5).generateBlend(participants);
    for (const Video& v : blend.getVideoList()) {
        EXPECT_FALSE(v.getVideoID().empty());
    }
}

TEST(RandomBlendAlgorithmTest, TwoUsersBlendCombinesBoth) {
    User u1 = makeUserWithWatchLater("user1", WATCH_LATER_CSV);
    User u2 = makeUserWithWatchLater("user2", WATCH_LATER_CSV);
    std::list<User> participants = {u1, u2};

    // With 2 users, each contributing up to 2, blend should have at most 4 videos
    Blend blend = RandomBlendAlgorithm(2).generateBlend(participants);
    EXPECT_LE(blend.size(), 4);
    EXPECT_GT(blend.size(), 0);
}
