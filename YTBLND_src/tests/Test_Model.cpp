#include <gtest/gtest.h>
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Friend.hpp"
#include "../ModelLayer/Video.hpp"
#include "../ModelLayer/VideoEntry.hpp"
#include "../ModelLayer/Channel.hpp"
#include "../ModelLayer/YouTubeData.hpp"
#include "../ModelLayer/Blend.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ModelLayer/Message.hpp"

// ── Helpers ───────────────────────────────────────────────────────────────────

// Build a minimal Video with sensible defaults so tests stay readable.
static Video makeVideo(const std::string& id = "v1",
                       const std::string& title = "Test Video",
                       const std::string& channelID = "ch1",
                       int duration = 120) {
    return Video(id, title, channelID, "http://thumb.example.com/" + id,
                 duration, {"tag1", "tag2"});
}

// Build a minimal User.
static User makeUser(const std::string& id       = "u1",
                     const std::string& username  = "alice",
                     const std::string& email     = "alice@example.com",
                     const std::string& password  = "secret") {
    return User(id, username, email, password);
}

// ── User ─────────────────────────────────────────────────────────────────────

TEST(UserTest, Constructor_StoresAllFields) {
    // Verify that the four constructor arguments are retrievable via getters.
    User u = makeUser("u1", "alice", "alice@example.com", "secret");
    EXPECT_EQ("u1",               u.getUserID());
    EXPECT_EQ("alice",            u.getUsername());
    EXPECT_EQ("alice@example.com",u.getEmail());
    EXPECT_EQ("secret",           u.getPassword());
}

TEST(UserTest, Setters_UpdateFields) {
    // Setters for mutable fields should overwrite the initial values.
    User u = makeUser();
    u.setUsername("bob");
    u.setEmail("bob@example.com");
    u.setPassword("newpass");
    EXPECT_EQ("bob",             u.getUsername());
    EXPECT_EQ("bob@example.com", u.getEmail());
    EXPECT_EQ("newpass",         u.getPassword());
}

TEST(UserTest, AddFriend_AppearsInFriendList) {
    // Adding a second user as a friend should make them appear in getFriends().
    User alice = makeUser("u1", "alice", "alice@example.com", "pw");
    User bob   = makeUser("u2", "bob",   "bob@example.com",   "pw");
    alice.addFriend(bob);

    auto friends = alice.getFriends();
    ASSERT_EQ(1u, friends.size());
    EXPECT_EQ("u2",  friends.front().getUserID());
    EXPECT_EQ("bob", friends.front().getDisplayName());
}

TEST(UserTest, AddFriend_NoDuplicates) {
    // Adding the same user twice should only result in one entry.
    User alice = makeUser("u1", "alice", "", "pw");
    User bob   = makeUser("u2", "bob",   "", "pw");
    alice.addFriend(bob);
    alice.addFriend(bob);
    EXPECT_EQ(1u, alice.getFriends().size());
}

TEST(UserTest, RemoveFriend_RemovesCorrectEntry) {
    // removeFriend() should delete only the targeted friend, leaving others.
    User alice = makeUser("u1", "alice", "", "pw");
    User bob   = makeUser("u2", "bob",   "", "pw");
    User carol = makeUser("u3", "carol", "", "pw");
    alice.addFriend(bob);
    alice.addFriend(carol);
    alice.removeFriend("u2");

    auto friends = alice.getFriends();
    ASSERT_EQ(1u, friends.size());
    EXPECT_EQ("u3", friends.front().getUserID());
}

TEST(UserTest, RemoveFriend_NonexistentID_NoOp) {
    // Removing an ID that was never added should not crash or alter the list.
    User alice = makeUser();
    alice.removeFriend("nobody");
    EXPECT_TRUE(alice.getFriends().empty());
}

// ── Friend ────────────────────────────────────────────────────────────────────

TEST(FriendTest, Constructor_StoresAllFields) {
    // Verify that Friend stores and returns the three constructor fields.
    Friend f("u2", "bob", "bob@example.com");
    EXPECT_EQ("u2",              f.getUserID());
    EXPECT_EQ("bob",             f.getDisplayName());
    EXPECT_EQ("bob@example.com", f.getEmail());
}

TEST(FriendTest, Setters_UpdateFields) {
    // Mutable fields displayName and email should be updatable via setters.
    Friend f("u2", "bob", "bob@example.com");
    f.setDisplayName("robert");
    f.setEmail("robert@example.com");
    EXPECT_EQ("robert",             f.getDisplayName());
    EXPECT_EQ("robert@example.com", f.getEmail());
}

// ── Video ─────────────────────────────────────────────────────────────────────

TEST(VideoTest, Constructor_StoresAllFields) {
    // All six Video fields should be returned correctly by their getters.
    Video v("v1", "My Video", "ch1", "http://thumb.example.com/v1", 300,
            {"music", "pop"});
    EXPECT_EQ("v1",                        v.getVideoID());
    EXPECT_EQ("My Video",                  v.getTitle());
    EXPECT_EQ("ch1",                       v.getChannelID());
    EXPECT_EQ("http://thumb.example.com/v1", v.getThumbnailURL());
    EXPECT_EQ(300,                         v.getDuration());
    EXPECT_EQ(2u,                          v.getTags().size());
}

// ── VideoEntry ────────────────────────────────────────────────────────────────

TEST(VideoEntryTest, Constructor_StoresAllFields) {
    // VideoEntry should store the wrapped Video plus watch metadata.
    Video v = makeVideo("v1", "Test", "ch1", 60);
    VideoEntry entry(v, 5, 1700000000);
    EXPECT_EQ("v1", entry.getVideo().getVideoID());
    EXPECT_EQ(5,    entry.getWatchCount());
    EXPECT_EQ(1700000000, entry.getLastWatched());
}

TEST(VideoEntryTest, Setters_UpdateWatchData) {
    // Watch count and last-watched timestamp should be mutable.
    Video v = makeVideo();
    VideoEntry entry(v, 1, 100);
    entry.setWatchCount(10);
    entry.setLastWatched(200);
    EXPECT_EQ(10,  entry.getWatchCount());
    EXPECT_EQ(200, entry.getLastWatched());
}

// ── Channel ───────────────────────────────────────────────────────────────────

TEST(ChannelTest, Constructor_StoresAllFields) {
    // Channel should store its ID, display name, and category list.
    Channel ch("ch1", "TechChannel", {"tech", "science"});
    EXPECT_EQ("ch1",         ch.getChannelID());
    EXPECT_EQ("TechChannel", ch.getDisplayName());
    EXPECT_EQ(2u,            ch.getCategories().size());
}

// ── YouTubeData ───────────────────────────────────────────────────────────────

TEST(YouTubeDataTest, DefaultConstruct_AllListsEmpty) {
    // A freshly constructed YouTubeData should have no entries in any list.
    YouTubeData yt;
    EXPECT_TRUE(yt.getWatchHistory().empty());
    EXPECT_TRUE(yt.getLikedVideos().empty());
    EXPECT_TRUE(yt.getSubscribedChannels().empty());
    EXPECT_TRUE(yt.getWatchLaterVideos().empty());
}

TEST(YouTubeDataTest, SetWatchHistory_RoundTrips) {
    // Setting the watch history should make it retrievable via the getter.
    YouTubeData yt;
    Video v = makeVideo();
    yt.setWatchHistory({VideoEntry(v, 3, 0)});
    ASSERT_EQ(1u, yt.getWatchHistory().size());
    EXPECT_EQ("v1", yt.getWatchHistory().front().getVideo().getVideoID());
}

TEST(YouTubeDataTest, SetLikedVideos_RoundTrips) {
    // Setting liked videos should be reflected by getLikedVideos().
    YouTubeData yt;
    yt.setLikedVideos({makeVideo("v2")});
    ASSERT_EQ(1u, yt.getLikedVideos().size());
    EXPECT_EQ("v2", yt.getLikedVideos().front().getVideoID());
}

TEST(YouTubeDataTest, SetWatchLaterVideos_RoundTrips) {
    // Setting watch-later videos should be reflected by getWatchLaterVideos().
    YouTubeData yt;
    yt.setWatchLaterVideos({makeVideo("v3")});
    ASSERT_EQ(1u, yt.getWatchLaterVideos().size());
    EXPECT_EQ("v3", yt.getWatchLaterVideos().front().getVideoID());
}

TEST(YouTubeDataTest, GetMostWatchedVideos_ReturnsSortedByWatchCount) {
    // getMostWatchedVideos(n) should return the top-n videos ordered by
    // descending watch count.
    YouTubeData yt;
    Video v1 = makeVideo("v1"); // watchCount = 1
    Video v2 = makeVideo("v2"); // watchCount = 5
    Video v3 = makeVideo("v3"); // watchCount = 3
    yt.setWatchHistory({
        VideoEntry(v1, 1, 0),
        VideoEntry(v2, 5, 0),
        VideoEntry(v3, 3, 0)
    });

    auto top2 = yt.getMostWatchedVideos(2);
    ASSERT_EQ(2u, top2.size());
    EXPECT_EQ("v2", top2.front().getVideoID()); // highest watch count first
    EXPECT_EQ("v3", top2.back().getVideoID());
}

TEST(YouTubeDataTest, GetMostWatchedVideos_NLargerThanHistory_ReturnsAll) {
    // Requesting more videos than exist should return only what is available.
    YouTubeData yt;
    yt.setWatchHistory({VideoEntry(makeVideo("v1"), 2, 0)});
    EXPECT_EQ(1u, yt.getMostWatchedVideos(10).size());
}

TEST(YouTubeDataTest, GetTopChannels_ReturnsSortedByAccumulatedWatchCount) {
    // getTopChannels(n) should rank subscribed channels by total watch count
    // across all videos belonging to each channel.
    YouTubeData yt;

    // ch1 has total watch count 1, ch2 has total watch count 5.
    Video v1 = makeVideo("v1", "T1", "ch1");
    Video v2 = makeVideo("v2", "T2", "ch2");
    yt.setWatchHistory({VideoEntry(v1, 1, 0), VideoEntry(v2, 5, 0)});
    yt.setSubscribedChannels({
        Channel("ch1", "ChannelOne", {}),
        Channel("ch2", "ChannelTwo", {})
    });

    auto top = yt.getTopChannels(2);
    ASSERT_EQ(2u, top.size());
    EXPECT_EQ("ch2", top.front().getChannelID()); // more-watched channel first
}

TEST(YouTubeDataTest, GetTopChannels_NLargerThanSubscriptions_ReturnsAll) {
    // Requesting more channels than subscribed should return only what exists.
    YouTubeData yt;
    yt.setSubscribedChannels({Channel("ch1", "C1", {})});
    EXPECT_EQ(1u, yt.getTopChannels(99).size());
}

// ── Blend ─────────────────────────────────────────────────────────────────────

TEST(BlendTest, Constructor_StoresAllFields) {
    // All Blend fields should be stored and retrievable immediately after
    // construction.
    User alice = makeUser("u1", "alice", "", "pw");
    Video v    = makeVideo();
    Blend b("blend1", "random", {alice}, {v});

    EXPECT_EQ("blend1", b.getBlendID());
    EXPECT_EQ("random", b.getAlgorithmUsed());
    EXPECT_EQ(1u,       b.getParticipants().size());
    EXPECT_EQ(1u,       b.getVideoList().size());
}

TEST(BlendTest, Size_ReflectsVideoListLength) {
    // size() should equal the number of videos in the blend.
    Blend b("b1", "algo", {makeUser()}, {makeVideo("v1"), makeVideo("v2")});
    EXPECT_EQ(2, b.size());
}

TEST(BlendTest, GetVideo_ValidIndex_ReturnsCorrectVideo) {
    // getVideo(i) should return the video at position i (0-based).
    Video v0 = makeVideo("v0");
    Video v1 = makeVideo("v1");
    Blend b("b1", "algo", {makeUser()}, {v0, v1});
    EXPECT_EQ("v0", b.getVideo(0).getVideoID());
    EXPECT_EQ("v1", b.getVideo(1).getVideoID());
}

TEST(BlendTest, GetVideo_OutOfRange_Throws) {
    // getVideo() with an out-of-range index should throw std::out_of_range.
    Blend b("b1", "algo", {makeUser()}, {makeVideo()});
    EXPECT_THROW(b.getVideo(5),  std::out_of_range);
    EXPECT_THROW(b.getVideo(-1), std::out_of_range);
}

TEST(BlendTest, SetVideoList_ReplacesExistingList) {
    // setVideoList() should replace all previous videos.
    Blend b("b1", "algo", {makeUser()}, {makeVideo("old")});
    b.setVideoList({makeVideo("new1"), makeVideo("new2")});
    ASSERT_EQ(2, b.size());
    EXPECT_EQ("new1", b.getVideo(0).getVideoID());
}

// ── ChatRoom ──────────────────────────────────────────────────────────────────

TEST(ChatRoomTest, Constructor_StoresBlendIDAndParticipants) {
    // The blend ID and participant list passed to the constructor should be
    // returned unchanged by the getters.
    ChatRoom room("blend1", {"u1", "u2"});
    EXPECT_EQ("blend1", room.getBlendID());
    EXPECT_EQ(2u, room.getParticipantIDs().size());
}

TEST(ChatRoomTest, InitialMessages_IsEmpty) {
    // A newly created ChatRoom should have no messages.
    ChatRoom room("b1", {"u1"});
    EXPECT_TRUE(room.getMessages().empty());
}

TEST(ChatRoomTest, AddMessage_AppearsInMessageList) {
    // After adding a message, it should appear in getMessages() with the
    // correct sender and text.
    ChatRoom room("b1", {"u1"});
    room.addMessage("u1", "Hello!");
    auto msgs = room.getMessages();
    ASSERT_EQ(1u, msgs.size());
    EXPECT_EQ("u1",    msgs.front().userID);
    EXPECT_EQ("Hello!", msgs.front().text);
}

TEST(ChatRoomTest, AddMessage_MultipleMessages_PreservesOrder) {
    // Messages should be stored in insertion order (oldest first).
    ChatRoom room("b1", {"u1", "u2"});
    room.addMessage("u1", "first");
    room.addMessage("u2", "second");
    auto msgs = room.getMessages();
    ASSERT_EQ(2u, msgs.size());
    auto it = msgs.begin();
    EXPECT_EQ("first",  it->text);
    EXPECT_EQ("second", (++it)->text);
}

TEST(ChatRoomTest, IsParticipant_ReturnsTrueForKnownUser) {
    // isParticipant() should return true for a userID that was passed to
    // the constructor.
    ChatRoom room("b1", {"u1", "u2"});
    EXPECT_TRUE(room.isParticipant("u1"));
    EXPECT_TRUE(room.isParticipant("u2"));
}

TEST(ChatRoomTest, IsParticipant_ReturnsFalseForUnknownUser) {
    // isParticipant() should return false for any userID not in the list.
    ChatRoom room("b1", {"u1"});
    EXPECT_FALSE(room.isParticipant("u99"));
}

TEST(ChatRoomTest, ClearMessages_EmptiesMessageList) {
    // clearMessages() should remove all previously added messages.
    ChatRoom room("b1", {"u1"});
    room.addMessage("u1", "hi");
    room.addMessage("u1", "bye");
    room.clearMessages();
    EXPECT_TRUE(room.getMessages().empty());
}
