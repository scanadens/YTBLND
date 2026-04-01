package database_layer

import (
	"path/filepath"
	"reflect"
	"sort"
	"testing"
)

// newTestManager creates an isolated SQLite file DB per test so data does not
// leak across test cases and each scenario is deterministic.
func newTestManager(t *testing.T) *SqliteDatabaseManager {
	t.Helper()
	dbPath := filepath.Join(t.TempDir(), "test.db")
	mgr, err := NewSqliteDataManager(dbPath)
	if err != nil {
		t.Fatalf("NewSqliteDataManager() error = %v", err)
	}

	// Close the underlying connection to avoid file descriptor buildup when
	// running multiple tests on low-memory instances.
	t.Cleanup(func() {
		if mgr.db != nil {
			_ = mgr.db.Close()
		}
	})

	return mgr
}

func TestSqliteDataManager_UserFlow(t *testing.T) {
	mgr := newTestManager(t)

	user := User{
		UserID:   "u-1",
		Username: "alice",
		Email:    "alice@example.com",
		Password: "pw123",
	}

	if err := mgr.CreateUser(user); err != nil {
		t.Fatalf("CreateUser() error = %v", err)
	}

	loaded, err := mgr.FindUserByID(user.UserID)
	if err != nil {
		t.Fatalf("FindUserByID() error = %v", err)
	}
	if loaded == nil {
		t.Fatalf("FindUserByID() = nil user, want populated user")
	}
	if *loaded != user {
		t.Fatalf("FindUserByID() = %#v, want %#v", *loaded, user)
	}

	if !mgr.ValidatePassword(user.UserID, user.Password) {
		t.Fatalf("ValidatePassword() = false, want true for correct password")
	}
	if mgr.ValidatePassword(user.UserID, "wrong") {
		t.Fatalf("ValidatePassword() = true, want false for wrong password")
	}
}

func TestSqliteDataManager_WatchLaterReplaceAndOrder(t *testing.T) {
	mgr := newTestManager(t)

	first := []Video{
		{VideoID: "v-1", Title: "First", ChannelID: "cid-1", ChannelName: "Chan One", ThumbnailURL: "thumb-1", ChannelLogoURL: "logo-1"},
		{VideoID: "v-2", Title: "Second", ChannelID: "cid-2", ChannelName: "Chan Two", ThumbnailURL: "thumb-2", ChannelLogoURL: "logo-2"},
	}
	if err := mgr.SaveWatchLater("u-1", first); err != nil {
		t.Fatalf("SaveWatchLater(first) error = %v", err)
	}

	replacement := []Video{
		{VideoID: "v-3", Title: "Third", ChannelID: "cid-3", ChannelName: "Chan Three", ThumbnailURL: "thumb-3", ChannelLogoURL: "logo-3"},
		{VideoID: "v-4", Title: "Fourth", ChannelID: "cid-4", ChannelName: "Chan Four", ThumbnailURL: "thumb-4", ChannelLogoURL: "logo-4"},
		{VideoID: "v-5", Title: "Fifth", ChannelID: "cid-5", ChannelName: "Chan Five", ThumbnailURL: "thumb-5", ChannelLogoURL: "logo-5"},
	}
	if err := mgr.SaveWatchLater("u-1", replacement); err != nil {
		t.Fatalf("SaveWatchLater(replacement) error = %v", err)
	}

	loaded, err := mgr.LoadWatchLater("u-1")
	if err != nil {
		t.Fatalf("LoadWatchLater() error = %v", err)
	}

	// Verify all persisted watch-later metadata fields and ordering.
	gotIDs := make([]string, 0, len(loaded))
	gotTitles := make([]string, 0, len(loaded))
	gotChannelIDs := make([]string, 0, len(loaded))
	gotChannelNames := make([]string, 0, len(loaded))
	gotThumbs := make([]string, 0, len(loaded))
	gotLogos := make([]string, 0, len(loaded))
	for _, v := range loaded {
		gotIDs = append(gotIDs, v.VideoID)
		gotTitles = append(gotTitles, v.Title)
		gotChannelIDs = append(gotChannelIDs, v.ChannelID)
		gotChannelNames = append(gotChannelNames, v.ChannelName)
		gotThumbs = append(gotThumbs, v.ThumbnailURL)
		gotLogos = append(gotLogos, v.ChannelLogoURL)
	}

	wantIDs := []string{"v-3", "v-4", "v-5"}
	wantTitles := []string{"Third", "Fourth", "Fifth"}
	wantChannelIDs := []string{"cid-3", "cid-4", "cid-5"}
	wantChannelNames := []string{"Chan Three", "Chan Four", "Chan Five"}
	wantThumbs := []string{"thumb-3", "thumb-4", "thumb-5"}
	wantLogos := []string{"logo-3", "logo-4", "logo-5"}
	if !reflect.DeepEqual(gotIDs, wantIDs) {
		t.Fatalf("LoadWatchLater() ids = %v, want %v", gotIDs, wantIDs)
	}
	if !reflect.DeepEqual(gotTitles, wantTitles) {
		t.Fatalf("LoadWatchLater() titles = %v, want %v", gotTitles, wantTitles)
	}
	if !reflect.DeepEqual(gotChannelIDs, wantChannelIDs) {
		t.Fatalf("LoadWatchLater() channel ids = %v, want %v", gotChannelIDs, wantChannelIDs)
	}
	if !reflect.DeepEqual(gotChannelNames, wantChannelNames) {
		t.Fatalf("LoadWatchLater() channel names = %v, want %v", gotChannelNames, wantChannelNames)
	}
	if !reflect.DeepEqual(gotThumbs, wantThumbs) {
		t.Fatalf("LoadWatchLater() thumbnail urls = %v, want %v", gotThumbs, wantThumbs)
	}
	if !reflect.DeepEqual(gotLogos, wantLogos) {
		t.Fatalf("LoadWatchLater() channel logo urls = %v, want %v", gotLogos, wantLogos)
	}
}

func TestSqliteDataManager_BlendFlow(t *testing.T) {
	mgr := newTestManager(t)

	participants := []string{"u-1", "u-2", "u-2", "u-3"}
	if err := mgr.SaveBlend("b-1", "u-1", "round_robin", participants); err != nil {
		t.Fatalf("SaveBlend() error = %v", err)
	}

	blendID, err := mgr.FindBlendForUser("u-2")
	if err != nil {
		t.Fatalf("FindBlendForUser() error = %v", err)
	}
	if blendID != "b-1" {
		t.Fatalf("FindBlendForUser() = %q, want %q", blendID, "b-1")
	}

	loaded, err := mgr.LoadBlendParticipants("b-1")
	if err != nil {
		t.Fatalf("LoadBlendParticipants() error = %v", err)
	}

	// SaveBlend uses INSERT OR IGNORE, so duplicate input IDs are de-duplicated.
	sort.Strings(loaded)
	want := []string{"u-1", "u-2", "u-3"}
	sort.Strings(want)
	if !reflect.DeepEqual(loaded, want) {
		t.Fatalf("LoadBlendParticipants() = %v, want %v", loaded, want)
	}

	chatRoomID, err := mgr.GetChatRoomForBlend("b-1")
	if err != nil {
		t.Fatalf("GetChatRoomForBlend() error = %v", err)
	}
	if chatRoomID != "b-1" {
		t.Fatalf("GetChatRoomForBlend() = %q, want %q", chatRoomID, "b-1")
	}

	isMember, err := mgr.IsChatRoomMember("b-1", "u-2")
	if err != nil {
		t.Fatalf("IsChatRoomMember() error = %v", err)
	}
	if !isMember {
		t.Fatalf("IsChatRoomMember() = false, want true")
	}

	chatMembers, err := mgr.LoadChatRoomMembers("b-1")
	if err != nil {
		t.Fatalf("LoadChatRoomMembers() error = %v", err)
	}
	sort.Strings(chatMembers)
	if !reflect.DeepEqual(chatMembers, want) {
		t.Fatalf("LoadChatRoomMembers() = %v, want %v", chatMembers, want)
	}
}

func TestSqliteDataManager_ChatHistory(t *testing.T) {
	mgr := newTestManager(t)

	if err := mgr.SaveBlend("b-chat", "u-1", "round_robin", []string{"u-1", "u-2"}); err != nil {
		t.Fatalf("SaveBlend() error = %v", err)
	}

	first := NewChatMessageRecord("b-chat", "u-1", "hello", "2026-03-29T04:30:00Z")
	second := NewChatMessageRecord("b-chat", "u-2", "hi back", "2026-03-29T04:30:01Z")
	third := NewChatMessageRecord("b-chat", "u-1", "follow up", "2026-03-29T04:30:02Z")

	if err := mgr.SaveChatMessage("b-chat", first); err != nil {
		t.Fatalf("SaveChatMessage(first) error = %v", err)
	}
	if err := mgr.SaveChatMessage("b-chat", second); err != nil {
		t.Fatalf("SaveChatMessage(second) error = %v", err)
	}
	if err := mgr.SaveChatMessage("b-chat", third); err != nil {
		t.Fatalf("SaveChatMessage(third) error = %v", err)
	}

	loaded, err := mgr.LoadChatMessages("b-chat", 2)
	if err != nil {
		t.Fatalf("LoadChatMessages() error = %v", err)
	}

	want := []ChatMessageRecord{second, third}
	if !reflect.DeepEqual(loaded, want) {
		t.Fatalf("LoadChatMessages() = %#v, want %#v", loaded, want)
	}
}
