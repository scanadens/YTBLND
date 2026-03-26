package routelayer

import (
	"net/http"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
)

type RegisterRequest struct {
	// UserID is the stable account identifier used across API and storage.
	UserID string `json:"user_id" binding:"required"`
	// Username is the public-facing display name for the account.
	Username string `json:"username" binding:"required"`
	// Email is optional contact metadata stored with the account.
	Email string `json:"email"`
	// Password is currently plain text for local development parity.
	Password string `json:"password" binding:"required"`
}

// LoginRequest captures credentials required to authenticate a user.
type LoginRequest struct {
	UserID   string `json:"user_id" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// AuthResponse is returned after register/login to provide user identity data.
type AuthResponse struct {
	UserID   string `json:"user_id"`
	Username string `json:"username"`
	Email    string `json:"email,omitempty"`
	// Token is reserved for future JWT/session auth integration.
	Token string `json:"token,omitempty"`
}

// AuthHandler wires auth HTTP handlers to the data layer.
type AuthHandler struct {
	dataManager database_layer.DataManager
}

// NewAuthHandler builds an auth handler set.
func NewAuthHandler(dataManager database_layer.DataManager) *AuthHandler {
	return &AuthHandler{dataManager: dataManager}
}

// RegisterAuthRoutes mounts auth endpoints under the provided router group.
func RegisterAuthRoutes(api *gin.RouterGroup, dataManager database_layer.DataManager) {
	h := NewAuthHandler(dataManager)
	auth := api.Group("/auth")
	auth.POST("/register", h.Register)
	auth.POST("/login", h.Login)
}

// Register creates a new user account.
func (h *AuthHandler) Register(c *gin.Context) {
	// Bind and validate incoming JSON in one call.
	var req RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Convert transport DTO into the data-layer model.
	user := database_layer.NewUser(req.UserID, req.Username, req.Email, req.Password)
	if err := h.dataManager.CreateUser(user); err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": "user already exists or could not be created"})
		return
	}

	c.JSON(http.StatusCreated, AuthResponse{
		UserID:   req.UserID,
		Username: req.Username,
		Email:    req.Email,
	})
}

// Login validates a user ID/password pair.
func (h *AuthHandler) Login(c *gin.Context) {
	// Validate required request fields from JSON body.
	var req LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Fast credential check before loading profile payload.
	if !h.dataManager.ValidatePassword(req.UserID, req.Password) {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}

	user, err := h.dataManager.FindUserByID(req.UserID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user profile"})
		return
	}
	if user == nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}

	c.JSON(http.StatusOK, AuthResponse{
		UserID:   user.GetUserID(),
		Username: user.GetUsername(),
		Email:    user.GetEmail(),
	})
}
