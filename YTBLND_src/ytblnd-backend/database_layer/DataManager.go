package database_layer

// DataManager defines the persistence operations consumed by higher layers.
// Each method is intentionally scoped to the current backend use cases so a
// different storage implementation can be swapped in without handler changes.
type DataManager interface {
	// CreateUser inserts a new user record and returns an error when the user
	// already exists or storage is unavailable.
	CreateUser(user User) error

	// FindUserByID returns the matched user, or nil,nil when no user exists.
	FindUserByID(id string) (*User, error)

	// ValidatePassword checks the stored credential for a given user ID.
	ValidatePassword(id, password string) bool

	// SaveWatchLater replaces the full watch-later list for a user.
	SaveWatchLater(userID string, videos []Video) error

	// LoadWatchLater returns videos in the same order they were saved.
	LoadWatchLater(userID string) ([]Video, error)

	// SaveBlend upserts blend metadata and overwrites participant membership.
	SaveBlend(blendID, creatorID, algorithm string, participants []string) error

	// FindBlendForUser returns the latest blend ID for a user, or an empty string.
	FindBlendForUser(userID string) (string, error)

	// LoadBlendParticipants returns all user IDs currently linked to a blend.
	LoadBlendParticipants(blendID string) ([]string, error)

	// GetChatRoomForBlend returns the chat room ID attached to a blend.
	// Empty string means no chat room exists for that blend.
	GetChatRoomForBlend(blendID string) (string, error)

	// IsChatRoomMember reports whether a user is a member of a chat room.
	IsChatRoomMember(chatRoomID, userID string) (bool, error)

	// LoadChatRoomMembers returns all members assigned to a chat room.
	LoadChatRoomMembers(chatRoomID string) ([]string, error)

	// SaveChatMessage appends one chat message to the room history store.
	SaveChatMessage(chatRoomID string, message ChatMessageRecord) error

	// LoadChatMessages returns the room message history ordered oldest to newest.
	LoadChatMessages(chatRoomID string, limit int) ([]ChatMessageRecord, error)
}
