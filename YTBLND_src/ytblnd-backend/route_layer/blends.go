package routelayer

import (
	"net/http"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
)

// VideoDTO is a transport-safe representation of video metadata used by routes.
type VideoDTO struct {
	VideoID        string `json:"video_id" binding:"required"`
	Title          string `json:"title"`
	ChannelID      string `json:"channel_id,omitempty"`
	ChannelName    string `json:"channel_name,omitempty"`
	ThumbnailURL   string `json:"thumbnail_url,omitempty"`
	ChannelLogoURL string `json:"channel_logo_url,omitempty"`
}

// SaveWatchLaterRequest replaces the entire watch-later list for a user.
type SaveWatchLaterRequest struct {
	Videos []VideoDTO `json:"videos" binding:"required"`
}

// CreateBlendRequest defines the payload required to create/update a blend.
type CreateBlendRequest struct {
	BlendID      string   `json:"blend_id" binding:"required"`
	CreatorID    string   `json:"creator_id" binding:"required"`
	Algorithm    string   `json:"algorithm" binding:"required"`
	Participants []string `json:"participants" binding:"required"`
}

// watchLaterResponse returns a user's current stored watch-later list.
type watchLaterResponse struct {
	UserID string     `json:"user_id"`
	Videos []VideoDTO `json:"videos"`
}

// blendSummaryResponse confirms blend persistence and linked chat room ID.
type blendSummaryResponse struct {
	BlendID    string `json:"blend_id"`
	ChatRoomID string `json:"chat_room_id"`
	Status     string `json:"status"`
}

// userBlendResponse maps a user to their latest blend association.
type userBlendResponse struct {
	UserID  string `json:"user_id"`
	BlendID string `json:"blend_id"`
}

// participantsResponse returns all known members for a blend.
type participantsResponse struct {
	BlendID      string   `json:"blend_id"`
	Participants []string `json:"participants"`
}

// blendChatRoomResponse returns the chat room attached to a blend plus members.
type blendChatRoomResponse struct {
	BlendID     string   `json:"blend_id"`
	ChatRoomID  string   `json:"chat_room_id"`
	MemberUsers []string `json:"member_users"`
}

// BlendHandler handles watch-later and blend metadata endpoints.
type BlendHandler struct {
	dataManager database_layer.DataManager
	logger      *EventLogger
}

// NewBlendHandler builds blend and watch-later handlers.
func NewBlendHandler(dataManager database_layer.DataManager, logger *EventLogger) *BlendHandler {
	return &BlendHandler{dataManager: dataManager, logger: logger}
}

// RegisterBlendRoutes mounts blend and watch-later endpoints.
func RegisterBlendRoutes(api *gin.RouterGroup, dataManager database_layer.DataManager, logger *EventLogger) {
	h := NewBlendHandler(dataManager, logger)
	api.POST("/users/:userID/watch-later", h.SaveWatchLater)
	api.GET("/users/:userID/watch-later", h.GetWatchLater)
	api.POST("/blends", h.CreateBlend)
	api.GET("/users/:userID/blend", h.GetBlendForUser)
	api.GET("/blends/:blendID/participants", h.GetBlendParticipants)
	api.GET("/blends/:blendID/chatroom", h.GetBlendChatRoom)
}

// SaveWatchLater replaces a user's watch-later list.
func (h *BlendHandler) SaveWatchLater(c *gin.Context) {
	userID := c.Param("userID")
	if userID == "" {
		h.logger.LogEvent("watch_later_rejected", "reason=missing_user_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	var req SaveWatchLaterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		h.logger.LogEvent("watch_later_rejected", "user_id=%s client_ip=%s error=%q", userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Translate route DTOs into database-layer video models.
	videos := make([]database_layer.Video, 0, len(req.Videos))
	for _, v := range req.Videos {
		videos = append(videos, database_layer.NewVideo(
			v.VideoID,
			v.Title,
			v.ChannelID,
			v.ChannelName,
			v.ThumbnailURL,
			v.ChannelLogoURL,
			0,
			nil,
		))
	}

	if err := h.dataManager.SaveWatchLater(userID, videos); err != nil {
		h.logger.LogEvent("watch_later_failed", "user_id=%s client_ip=%s error=%q", userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to save watch-later list"})
		return
	}
	h.logger.LogEvent("watch_later_saved", "user_id=%s saved_count=%d client_ip=%s", userID, len(req.Videos), c.ClientIP())

	c.JSON(http.StatusOK, gin.H{
		"user_id":        userID,
		"saved_count":    len(req.Videos),
		"watch_later_ok": true,
	})
}

// GetWatchLater loads a user's watch-later videos in stored order.
func (h *BlendHandler) GetWatchLater(c *gin.Context) {
	userID := c.Param("userID")
	if userID == "" {
		h.logger.LogEvent("watch_later_load_rejected", "reason=missing_user_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	videos, err := h.dataManager.LoadWatchLater(userID)
	if err != nil {
		h.logger.LogEvent("watch_later_load_failed", "user_id=%s client_ip=%s error=%q", userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load watch-later list"})
		return
	}
	h.logger.LogEvent("watch_later_loaded", "user_id=%s count=%d client_ip=%s", userID, len(videos), c.ClientIP())

	// Map persistence models to response DTOs used by the API contract.
	respVideos := make([]VideoDTO, 0, len(videos))
	for _, v := range videos {
		respVideos = append(respVideos, VideoDTO{
			VideoID:        v.GetVideoID(),
			Title:          v.GetTitle(),
			ChannelID:      v.GetChannelID(),
			ChannelName:    v.GetChannelName(),
			ThumbnailURL:   v.GetThumbnailURL(),
			ChannelLogoURL: v.GetChannelLogoURL(),
		})
	}

	c.JSON(http.StatusOK, watchLaterResponse{
		UserID: userID,
		Videos: respVideos,
	})
}

// CreateBlend saves blend metadata and participant membership.
func (h *BlendHandler) CreateBlend(c *gin.Context) {
	var req CreateBlendRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		h.logger.LogEvent("blend_save_rejected", "reason=invalid_payload client_ip=%s error=%q", c.ClientIP(), err.Error())
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// SaveBlend also ensures a blend-attached chat room exists and is synced.
	if err := h.dataManager.SaveBlend(req.BlendID, req.CreatorID, req.Algorithm, req.Participants); err != nil {
		h.logger.LogEvent("blend_save_failed", "blend_id=%s creator_id=%s client_ip=%s error=%q", req.BlendID, req.CreatorID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to save blend"})
		return
	}

	chatRoomID, err := h.dataManager.GetChatRoomForBlend(req.BlendID)
	if err != nil {
		h.logger.LogEvent("blend_chat_lookup_failed", "blend_id=%s client_ip=%s error=%q", req.BlendID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend chat room"})
		return
	}
	h.logger.LogEvent("blend_saved", "blend_id=%s creator_id=%s participants=%d chat_room_id=%s client_ip=%s", req.BlendID, req.CreatorID, len(req.Participants), chatRoomID, c.ClientIP())

	c.JSON(http.StatusOK, blendSummaryResponse{
		BlendID:    req.BlendID,
		ChatRoomID: chatRoomID,
		Status:     "saved",
	})
}

// GetBlendForUser returns the latest blend ID for a user.
func (h *BlendHandler) GetBlendForUser(c *gin.Context) {
	userID := c.Param("userID")
	if userID == "" {
		h.logger.LogEvent("blend_lookup_rejected", "reason=missing_user_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	blendID, err := h.dataManager.FindBlendForUser(userID)
	if err != nil {
		h.logger.LogEvent("blend_lookup_failed", "user_id=%s client_ip=%s error=%q", userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to query blend for user"})
		return
	}
	if blendID == "" {
		h.logger.LogEvent("blend_lookup_empty", "user_id=%s client_ip=%s", userID, c.ClientIP())
		c.JSON(http.StatusNotFound, gin.H{"error": "no blend found for user"})
		return
	}
	h.logger.LogEvent("blend_lookup_succeeded", "user_id=%s blend_id=%s client_ip=%s", userID, blendID, c.ClientIP())

	c.JSON(http.StatusOK, userBlendResponse{
		UserID:  userID,
		BlendID: blendID,
	})
}

// GetBlendParticipants returns all participants in a blend.
func (h *BlendHandler) GetBlendParticipants(c *gin.Context) {
	blendID := c.Param("blendID")
	if blendID == "" {
		h.logger.LogEvent("participants_rejected", "reason=missing_blend_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path parameter is required"})
		return
	}

	participants, err := h.dataManager.LoadBlendParticipants(blendID)
	if err != nil {
		h.logger.LogEvent("participants_failed", "blend_id=%s client_ip=%s error=%q", blendID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend participants"})
		return
	}
	h.logger.LogEvent("participants_loaded", "blend_id=%s count=%d client_ip=%s", blendID, len(participants), c.ClientIP())

	c.JSON(http.StatusOK, participantsResponse{
		BlendID:      blendID,
		Participants: participants,
	})
}

// GetBlendChatRoom returns the chat room attached to a blend and its members.
func (h *BlendHandler) GetBlendChatRoom(c *gin.Context) {
	blendID := c.Param("blendID")
	if blendID == "" {
		h.logger.LogEvent("chatroom_lookup_rejected", "reason=missing_blend_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path parameter is required"})
		return
	}

	chatRoomID, err := h.dataManager.GetChatRoomForBlend(blendID)
	if err != nil {
		h.logger.LogEvent("chatroom_lookup_failed", "blend_id=%s client_ip=%s error=%q", blendID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend chat room"})
		return
	}
	if chatRoomID == "" {
		h.logger.LogEvent("chatroom_lookup_empty", "blend_id=%s client_ip=%s", blendID, c.ClientIP())
		c.JSON(http.StatusNotFound, gin.H{"error": "no chat room found for blend"})
		return
	}

	members, err := h.dataManager.LoadChatRoomMembers(chatRoomID)
	if err != nil {
		h.logger.LogEvent("chatroom_members_failed", "blend_id=%s chat_room_id=%s client_ip=%s error=%q", blendID, chatRoomID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load chat room members"})
		return
	}
	h.logger.LogEvent("chatroom_loaded", "blend_id=%s chat_room_id=%s members=%d client_ip=%s", blendID, chatRoomID, len(members), c.ClientIP())

	c.JSON(http.StatusOK, blendChatRoomResponse{
		BlendID:     blendID,
		ChatRoomID:  chatRoomID,
		MemberUsers: members,
	})
}
