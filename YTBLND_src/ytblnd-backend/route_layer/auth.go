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

// DeleteAccountRequest captures the password for re-authentication before deletion.
type DeleteAccountRequest struct {
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
	logger      *EventLogger
}

// NewAuthHandler builds an auth handler set.
func NewAuthHandler(dataManager database_layer.DataManager, logger *EventLogger) *AuthHandler {
	return &AuthHandler{dataManager: dataManager, logger: logger}
}

// RegisterAuthRoutes mounts auth endpoints under the provided router group.
func RegisterAuthRoutes(api *gin.RouterGroup, dataManager database_layer.DataManager, logger *EventLogger) {
	h := NewAuthHandler(dataManager, logger)
	auth := api.Group("/auth")
	auth.POST("/register", h.Register)
	auth.POST("/login", h.Login)
	auth.GET("/users/:userID", h.GetUser)
	auth.DELETE("/users/:userID", h.DeleteAccount)
}

// Register creates a new user account.
func (h *AuthHandler) Register(c *gin.Context) {
	// Bind and validate incoming JSON in one call.
	var req RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		h.logger.LogEvent("auth_register_rejected", "reason=invalid_payload client_ip=%s error=%q", c.ClientIP(), err.Error())
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Convert transport DTO into the data-layer model.
	user := database_layer.NewUser(req.UserID, req.Username, req.Email, req.Password)
	if err := h.dataManager.CreateUser(user); err != nil {
		h.logger.LogEvent("auth_register_failed", "user_id=%s client_ip=%s error=%q", req.UserID, c.ClientIP(), err.Error())
		c.JSON(http.StatusConflict, gin.H{"error": "user already exists or could not be created"})
		return
	}
	h.logger.LogEvent("auth_register_succeeded", "user_id=%s username=%s client_ip=%s", req.UserID, req.Username, c.ClientIP())

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
		h.logger.LogEvent("auth_login_rejected", "reason=invalid_payload client_ip=%s error=%q", c.ClientIP(), err.Error())
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Fast credential check before loading profile payload.
	if !h.dataManager.ValidatePassword(req.UserID, req.Password) {
		h.logger.LogEvent("auth_login_failed", "user_id=%s client_ip=%s reason=invalid_credentials", req.UserID, c.ClientIP())
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}

	user, err := h.dataManager.FindUserByID(req.UserID)
	if err != nil {
		h.logger.LogEvent("auth_login_error", "user_id=%s client_ip=%s error=%q", req.UserID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user profile"})
		return
	}
	if user == nil {
		h.logger.LogEvent("auth_login_failed", "user_id=%s client_ip=%s reason=missing_profile", req.UserID, c.ClientIP())
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}
	h.logger.LogEvent("auth_login_succeeded", "user_id=%s client_ip=%s", req.UserID, c.ClientIP())

	c.JSON(http.StatusOK, AuthResponse{
		UserID:   user.GetUserID(),
		Username: user.GetUsername(),
		Email:    user.GetEmail(),
	})
}

// GetUser returns user profile fields for a specific user ID.
func (h *AuthHandler) GetUser(c *gin.Context) {
	userID := c.Param("userID")
	if userID == "" {
		h.logger.LogEvent("auth_user_lookup_rejected", "reason=missing_user_id client_ip=%s", c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	// Direct profile lookup allows clients to fetch user identity without side-effect routes.
	user, err := h.dataManager.FindUserByID(userID)
	if err != nil {
		h.logger.LogEvent("auth_user_lookup_failed", "user_id=%s client_ip=%s error=%q", userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user profile"})
		return
	}
	if user == nil {
		h.logger.LogEvent("auth_user_lookup_missing", "user_id=%s client_ip=%s", userID, c.ClientIP())
		c.JSON(http.StatusNotFound, gin.H{"error": "user not found"})
		return
	}
	h.logger.LogEvent("auth_user_lookup_succeeded", "user_id=%s client_ip=%s", userID, c.ClientIP())

	c.JSON(http.StatusOK, AuthResponse{
		UserID:   user.GetUserID(),
		Username: user.GetUsername(),
		Email:    user.GetEmail(),
	})
}

// DeleteAccount removes a user account after password re-authentication.
func (h *AuthHandler) DeleteAccount(c *gin.Context) {
	userID := c.Param("userID")
	if userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "userID path parameter is required"})
		return
	}

	var req DeleteAccountRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if err := h.dataManager.DeleteUser(userID, req.Password); err != nil {
		h.logger.LogEvent("auth_delete_failed", "user_id=%s error=%q", userID, err.Error())
		if err.Error() == "invalid credentials" {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to delete account"})
		return
	}
	h.logger.LogEvent("auth_delete_succeeded", "user_id=%s", userID)

	c.JSON(http.StatusOK, gin.H{"user_id": userID, "status": "deleted"})
}
