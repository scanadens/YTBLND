/**
 * \file Test_WatchLaterBlend.cpp
 * \brief Unit tests for watch-later parsing and blend generation inputs.
 * \author Jasmine Kumar
 *
 * \details
 * Validates data-ingestion and blend-input behavior for:
 * - WatchLaterParser extraction from fixture CSV files.
 * - WatchHistoryParser extraction from fixture HTML files.
 * - YouTubeDataImporter extension routing and unsupported-file handling.
 * - RandomBlendAlgorithm bounds/selection behavior using parsed watch-later inputs.
 */

#include "gtest/gtest.h"

#include "../AppInfrastructure/WatchHistoryParser.hpp"
#include "../AppInfrastructure/WatchLaterParser.hpp"
#include "../AppInfrastructure/YouTubeDataImporter.hpp"
#include "../AlgorithmLayer/RandomBlendAlgorithm.hpp"
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/YouTubeData.hpp"
#include "../ModelLayer/Blend.hpp"

#include <list>
#include <stdexcept>
#include <string>

#ifndef YTBLND_SRC_DIR
#define YTBLND_SRC_DIR "."
#endif

// Synthetic fixture - does not contain personal data
static const std::string WATCH_LATER_CSV =
    // Use the committed fixture filename exactly as it appears in the repo.
    YTBLND_SRC_DIR "/tests/fixtures/jas_watch_later-videos.csv";

// Synthetic fixture - does not contain personal data
static const std::string WATCH_HISTORY_HTML =
    YTBLND_SRC_DIR "/tests/fixtures/shamar_sample_watch-history.html";

// WatchLaterParser

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

// WatchHistoryParser

TEST(WatchHistoryParserTest, ParsesNonEmptyList) {
    std::list<Video> videos = WatchHistoryParser(WATCH_HISTORY_HTML).parse();
    EXPECT_FALSE(videos.empty()) << "Expected at least one video in watch-history HTML";
}

TEST(WatchHistoryParserTest, EachVideoHasNonEmptyID) {
    std::list<Video> videos = WatchHistoryParser(WATCH_HISTORY_HTML).parse();
    for (const Video& v : videos) {
        EXPECT_FALSE(v.getVideoID().empty()) << "Found a Video with an empty videoID";
    }
}

// YouTubeDataImporter

TEST(YouTubeDataImporterTest, ImportsCsvAndHtmlFixtures) {
    YouTubeDataImporter importer;

    std::list<Video> csvVideos = importer.import(WATCH_LATER_CSV);
    std::list<Video> htmlVideos = importer.import(WATCH_HISTORY_HTML);

    EXPECT_FALSE(csvVideos.empty());
    EXPECT_FALSE(htmlVideos.empty());
}

TEST(YouTubeDataImporterTest, ThrowsOnUnsupportedExtension) {
    YouTubeDataImporter importer;
    EXPECT_THROW(importer.import("fixtures/not_supported.txt"), std::invalid_argument);
}

// RandomBlendAlgorithm

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
