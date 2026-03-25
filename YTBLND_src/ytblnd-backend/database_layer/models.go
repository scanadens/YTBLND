package database_layer

import "fmt"

// User is the persisted account model used by the data layer.
// Fields intentionally align with the users table and legacy C++ model.
type User struct {
	// UserID is the stable primary key used for lookups and joins.
	UserID string
	// Username is the public-facing display identifier.
	Username string
	// Email is optional in storage and may be empty.
	Email string
	// Password currently mirrors legacy plaintext behavior.
	Password string // keep for parity with current C++ behavior
}

// NewUser centralizes user construction so call sites stay consistent.
func NewUser(userID, username, email, password string) User {
	return User{
		UserID:   userID,
		Username: username,
		Email:    email,
		Password: password,
	}
}

// Getter helpers preserve parity with the C++ object-style API.
func (u User) GetUserID() string   { return u.UserID }
func (u User) GetUsername() string { return u.Username }
func (u User) GetEmail() string    { return u.Email }
func (u User) GetPassword() string { return u.Password }

// Video stores blend/watch-later metadata used by current persistence flows.
type Video struct {
	// VideoID is the external provider identifier (for example YouTube ID).
	VideoID string
	// Title is displayed in watch-later and blend outputs.
	Title string
	// ChannelID identifies the source channel/uploader.
	ChannelID string
	// ThumbnailURL points to a preview image when available.
	ThumbnailURL string
	// Duration is stored in seconds.
	Duration int
	// Tags are copied defensively to avoid accidental external mutation.
	Tags []string
}

// NewVideo clones tag input to keep model state isolated from caller slices.
func NewVideo(videoID, title, channelID, thumbnailURL string, duration int, tags []string) Video {
	t := append([]string(nil), tags...)
	return Video{
		VideoID:      videoID,
		Title:        title,
		ChannelID:    channelID,
		ThumbnailURL: thumbnailURL,
		Duration:     duration,
		Tags:         t,
	}
}

// Getter helpers keep direct field usage optional in higher layers.
func (v Video) GetVideoID() string      { return v.VideoID }
func (v Video) GetTitle() string        { return v.Title }
func (v Video) GetChannelID() string    { return v.ChannelID }
func (v Video) GetThumbnailURL() string { return v.ThumbnailURL }
func (v Video) GetDuration() int        { return v.Duration }

// GetTags returns a copy so callers cannot mutate model internals by reference.
func (v Video) GetTags() []string { return append([]string(nil), v.Tags...) }

// Blend groups a generated blend result with its participants and video list.
type Blend struct {
	// BlendID is the storage key for blend metadata.
	BlendID string
	// AlgorithmUsed records which strategy produced this blend.
	AlgorithmUsed string
	// Participants is the set of users included in the blend.
	Participants []User
	// VideoList is the ordered output playlist for the blend.
	VideoList []Video
}

// NewBlend copies participant and video slices to prevent aliasing bugs.
func NewBlend(blendID, algorithmUsed string, participants []User, videoList []Video) Blend {
	// Defensive copy to avoid accidental shared backing arrays.
	p := append([]User(nil), participants...)
	v := append([]Video(nil), videoList...)

	return Blend{
		BlendID:       blendID,
		AlgorithmUsed: algorithmUsed,
		Participants:  p,
		VideoList:     v,
	}
}

// Primary blend metadata accessors.
func (b Blend) GetBlendID() string       { return b.BlendID }
func (b Blend) GetAlgorithmUsed() string { return b.AlgorithmUsed }

// GetParticipants returns a copy to keep Blend state immutable to callers.
func (b Blend) GetParticipants() []User {
	return append([]User(nil), b.Participants...)
}

// GetVideoList returns a copy so external code cannot reorder internal state.
func (b Blend) GetVideoList() []Video {
	return append([]Video(nil), b.VideoList...)
}

// SetVideoList replaces the current list using a defensive copy.
func (b *Blend) SetVideoList(videoList []Video) {
	b.VideoList = append([]Video(nil), videoList...)
}

// GetVideo safely retrieves one video with bounds validation.
func (b Blend) GetVideo(index int) (Video, error) {
	if index < 0 || index >= len(b.VideoList) {
		return Video{}, fmt.Errorf("Blend.GetVideo: index %d out of range (size=%d)", index, len(b.VideoList))
	}
	return b.VideoList[index], nil
}

// Size returns the number of videos currently in the blend output.
func (b Blend) Size() int {
	return len(b.VideoList)
}
