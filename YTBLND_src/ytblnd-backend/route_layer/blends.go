package routelayer

import (
	"net/http"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
)

// VideoDTO is a transport-safe representation of video metadata used by routes.
type VideoDTO struct {
	VideoID string `json:"video_id" binding:"required"`
	Title   string `json:"title"`
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
}

// NewBlendHandler builds blend and watch-later handlers.
func NewBlendHandler(dataManager database_layer.DataManager) *BlendHandler {
	return &BlendHandler{dataManager: dataManager}
}

// RegisterBlendRoutes mounts blend and watch-later endpoints.
func RegisterBlendRoutes(api *gin.RouterGroup, dataManager database_layer.DataManager) {
	h := NewBlendHandler(dataManager)
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
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	var req SaveWatchLaterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Translate route DTOs into database-layer video models.
	videos := make([]database_layer.Video, 0, len(req.Videos))
	for _, v := range req.Videos {
		videos = append(videos, database_layer.NewVideo(v.VideoID, v.Title, "", "", 0, nil))
	}

	if err := h.dataManager.SaveWatchLater(userID, videos); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to save watch-later list"})
		return
	}

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
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	videos, err := h.dataManager.LoadWatchLater(userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load watch-later list"})
		return
	}

	// Map persistence models to response DTOs used by the API contract.
	respVideos := make([]VideoDTO, 0, len(videos))
	for _, v := range videos {
		respVideos = append(respVideos, VideoDTO{
			VideoID: v.GetVideoID(),
			Title:   v.GetTitle(),
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
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// SaveBlend also ensures a blend-attached chat room exists and is synced.
	if err := h.dataManager.SaveBlend(req.BlendID, req.CreatorID, req.Algorithm, req.Participants); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to save blend"})
		return
	}

	chatRoomID, err := h.dataManager.GetChatRoomForBlend(req.BlendID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend chat room"})
		return
	}

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
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	blendID, err := h.dataManager.FindBlendForUser(userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to query blend for user"})
		return
	}
	if blendID == "" {
		c.JSON(http.StatusNotFound, gin.H{"error": "no blend found for user"})
		return
	}

	c.JSON(http.StatusOK, userBlendResponse{
		UserID:  userID,
		BlendID: blendID,
	})
}

// GetBlendParticipants returns all participants in a blend.
func (h *BlendHandler) GetBlendParticipants(c *gin.Context) {
	blendID := c.Param("blendID")
	if blendID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path parameter is required"})
		return
	}

	participants, err := h.dataManager.LoadBlendParticipants(blendID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend participants"})
		return
	}

	c.JSON(http.StatusOK, participantsResponse{
		BlendID:      blendID,
		Participants: participants,
	})
}

// GetBlendChatRoom returns the chat room attached to a blend and its members.
func (h *BlendHandler) GetBlendChatRoom(c *gin.Context) {
	blendID := c.Param("blendID")
	if blendID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path parameter is required"})
		return
	}

	chatRoomID, err := h.dataManager.GetChatRoomForBlend(blendID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load blend chat room"})
		return
	}
	if chatRoomID == "" {
		c.JSON(http.StatusNotFound, gin.H{"error": "no chat room found for blend"})
		return
	}

	members, err := h.dataManager.LoadChatRoomMembers(chatRoomID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load chat room members"})
		return
	}

	c.JSON(http.StatusOK, blendChatRoomResponse{
		BlendID:     blendID,
		ChatRoomID:  chatRoomID,
		MemberUsers: members,
	})
}
