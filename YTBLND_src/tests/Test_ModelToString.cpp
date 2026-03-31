#include <gtest/gtest.h>

#include "../ModelLayer/Blend.hpp"
#include "../ModelLayer/Channel.hpp"
#include "../ModelLayer/ChatRoom.hpp"
#include "../ModelLayer/Friend.hpp"
#include "../ModelLayer/Message.hpp"
#include "../ModelLayer/User.hpp"
#include "../ModelLayer/Video.hpp"
#include "../ModelLayer/VideoEntry.hpp"
#include "../ModelLayer/YouTubeData.hpp"

#include <list>
#include <regex>
#include <string>

namespace {

void expectContains(const std::string& body, const std::string& token) {
    EXPECT_NE(body.find(token), std::string::npos) << "Missing token: " << token << " in " << body;
}

Video makeVideo(const std::string& id = "vid1", const std::string& title = "Video One") {
    return Video(id, title, "ch1", "https://img.example.com/" + id, 123, {"tagA", "tagB"});
}

} // namespace

TEST(ModelToStringJsonTest, FriendUsesAuthUserFields) {
    Friend f("u2", "bob", "bob@example.com");

    const std::string json = f.toString();
    EXPECT_EQ(json, "{\"user_id\":\"u2\",\"username\":\"bob\",\"email\":\"bob@example.com\"}");
}

TEST(ModelToStringJsonTest, VideoIncludesWatchLaterCompatibleFields) {
    Video v = makeVideo("vid2", "Video Two");

    const std::string json = v.toString();
    expectContains(json, "\"video_id\":\"vid2\"");
    expectContains(json, "\"title\":\"Video Two\"");
    expectContains(json, "\"channel_id\":\"ch1\"");
}

TEST(ModelToStringJsonTest, MessageUsesInboundChatSchema) {
    Message message("u2", "hello", 0);

    const std::string json = message.toString();
    EXPECT_EQ(json,
              "{\"sender_id\":\"u2\",\"content\":\"hello\",\"sent_at\":\"1970-01-01T00:00:00Z\"}");
}

TEST(ModelToStringJsonTest, BlendUsesBlendEndpointKeys) {
    User creator("u1", "alice", "alice@example.com", "pw");
    Blend blend("b1", "round_robin", {creator}, {makeVideo()});

    const std::string json = blend.toString();
    expectContains(json, "\"blend_id\":\"b1\"");
    expectContains(json, "\"chat_room_id\":\"b1\"");
    expectContains(json, "\"algorithm\":\"round_robin\"");
    expectContains(json, "\"participants\":[\"u1\"]");
    expectContains(json, "\"videos\":[{");
}

TEST(ModelToStringJsonTest, ChatRoomUsesChatroomAndMessageEnvelopeKeys) {
    ChatRoom room("b1", {"u1", "u2"});
    room.addMessage("u2", "hello from socket");

    const std::string json = room.toString();
    expectContains(json, "\"blend_id\":\"b1\"");
    expectContains(json, "\"chat_room_id\":\"b1\"");
    expectContains(json, "\"member_users\":[\"u1\",\"u2\"]");
    expectContains(json, "\"type\":\"chat_message\"");
    expectContains(json, "\"room_id\":\"b1\"");
    expectContains(json, "\"sender_id\":\"u2\"");
    expectContains(json, "\"content\":\"hello from socket\"");

    std::regex sentAtPattern("\\\"sent_at\\\":\\\"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z\\\"");
    EXPECT_TRUE(std::regex_search(json, sentAtPattern));
}

TEST(ModelToStringJsonTest, YouTubeDataWrapsWatchLaterVideosArray) {
    YouTubeData yt;
    yt.setWatchLaterVideos({makeVideo("vid9", "Video Nine")});

    const std::string json = yt.toString();
    expectContains(json, "\"watch_later\":{\"videos\":[{");
    expectContains(json, "\"video_id\":\"vid9\"");
    expectContains(json, "\"title\":\"Video Nine\"");
}

TEST(ModelToStringJsonTest, UserUsesAuthUserKeysAndNestedYouTubeData) {
    User user("u1", "alice", "alice@example.com", "pw");
    User friendUser("u2", "bob", "bob@example.com", "pw");
    user.addFriend(friendUser);

    const std::string json = user.toString();
    expectContains(json, "\"user_id\":\"u1\"");
    expectContains(json, "\"username\":\"alice\"");
    expectContains(json, "\"email\":\"alice@example.com\"");
    expectContains(json, "\"friends\":[{\"user_id\":\"u2\"");
    expectContains(json, "\"youtube_data\":{");
}
