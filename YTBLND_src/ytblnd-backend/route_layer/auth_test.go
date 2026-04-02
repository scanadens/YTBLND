package routelayer

import (
	"bytes"
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"testing"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
)

type fakeAuthDataManager struct {
	findUserByIDFn     func(id string) (*database_layer.User, error)
	validatePasswordFn func(id, password string) bool
	deleteUserFn       func(userID, password string) error

	findCalls     int
	validateCalls int
	deleteCalls   int
	lastFindID    string
	lastDeleteID  string
	lastDeletePw  string
}

func (f *fakeAuthDataManager) CreateUser(user database_layer.User) error { return nil }

func (f *fakeAuthDataManager) FindUserByID(id string) (*database_layer.User, error) {
	f.findCalls++
	f.lastFindID = id
	if f.findUserByIDFn == nil {
		return nil, nil
	}
	return f.findUserByIDFn(id)
}

func (f *fakeAuthDataManager) ValidatePassword(id, password string) bool {
	f.validateCalls++
	if f.validatePasswordFn == nil {
		return false
	}
	return f.validatePasswordFn(id, password)
}

func (f *fakeAuthDataManager) SaveWatchLater(userID string, videos []database_layer.Video) error {
	return nil
}

func (f *fakeAuthDataManager) LoadWatchLater(userID string) ([]database_layer.Video, error) {
	return nil, nil
}

func (f *fakeAuthDataManager) SaveBlend(blendID, title, creatorID, algorithm string, participants []string) error {
	return nil
}

func (f *fakeAuthDataManager) FindBlendForUser(userID string) (string, error) { return "", nil }

func (f *fakeAuthDataManager) FindAllBlendsForUser(userID string) ([]database_layer.BlendSummary, error) {
	return nil, nil
}

func (f *fakeAuthDataManager) LoadBlendParticipants(blendID string) ([]string, error) {
	return nil, nil
}

func (f *fakeAuthDataManager) RemoveParticipantFromBlend(blendID, userID string) error { return nil }

func (f *fakeAuthDataManager) GetChatRoomForBlend(blendID string) (string, error) { return "", nil }

func (f *fakeAuthDataManager) IsChatRoomMember(chatRoomID, userID string) (bool, error) {
	return false, nil
}

func (f *fakeAuthDataManager) LoadChatRoomMembers(chatRoomID string) ([]string, error) {
	return nil, nil
}

func (f *fakeAuthDataManager) SaveChatMessage(chatRoomID string, message database_layer.ChatMessageRecord) error {
	return nil
}

func (f *fakeAuthDataManager) LoadChatMessages(chatRoomID string, limit int) ([]database_layer.ChatMessageRecord, error) {
	return nil, nil
}

func (f *fakeAuthDataManager) DeleteUser(userID, password string) error {
	f.deleteCalls++
	f.lastDeleteID = userID
	f.lastDeletePw = password
	if f.deleteUserFn == nil {
		return nil
	}
	return f.deleteUserFn(userID, password)
}

func TestAuthDeleteUserRoute(t *testing.T) {
	gin.SetMode(gin.TestMode)

	tests := []struct {
		name            string
		pathUserID      string
		body            map[string]string
		deleteUser      func(userID, password string) error
		wantStatus      int
		wantDeleteCalls int
	}{
		{
			name:       "successfully deletes user",
			pathUserID: "u-123",
			body: map[string]string{
				"password": "pw-ok",
			},
			deleteUser: func(userID, password string) error {
				if userID != "u-123" {
					t.Fatalf("DeleteUser userID = %q, want %q", userID, "u-123")
				}
				if password != "pw-ok" {
					t.Fatalf("DeleteUser password = %q, want %q", password, "pw-ok")
				}
				return nil
			},
			wantStatus:      http.StatusOK,
			wantDeleteCalls: 1,
		},
		{
			name:       "rejects invalid credentials",
			pathUserID: "u-123",
			body: map[string]string{
				"password": "wrong",
			},
			deleteUser: func(userID, password string) error {
				return errors.New("invalid credentials")
			},
			wantStatus:      http.StatusUnauthorized,
			wantDeleteCalls: 1,
		},
		{
			name:            "rejects invalid payload",
			pathUserID:      "u-123",
			body:            map[string]string{},
			deleteUser:      func(userID, password string) error { return nil },
			wantStatus:      http.StatusBadRequest,
			wantDeleteCalls: 0,
		},
		{
			name:       "maps delete failures to internal error",
			pathUserID: "u-123",
			body: map[string]string{
				"password": "pw-ok",
			},
			deleteUser: func(userID, password string) error {
				return errors.New("user not found")
			},
			wantStatus:      http.StatusInternalServerError,
			wantDeleteCalls: 1,
		},
		{
			name:       "maps unexpected delete failure",
			pathUserID: "u-123",
			body: map[string]string{
				"password": "pw-ok",
			},
			deleteUser: func(userID, password string) error {
				return errors.New("boom")
			},
			wantStatus:      http.StatusInternalServerError,
			wantDeleteCalls: 1,
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			fakeDM := &fakeAuthDataManager{
				deleteUserFn: tc.deleteUser,
			}

			router := gin.New()
			api := router.Group("/api/v1")
			RegisterAuthRoutes(api, fakeDM, &EventLogger{})

			payload, err := json.Marshal(tc.body)
			if err != nil {
				t.Fatalf("json.Marshal(body) error = %v", err)
			}

			req := httptest.NewRequest(http.MethodDelete, "/api/v1/auth/users/"+tc.pathUserID, bytes.NewReader(payload))
			req.Header.Set("Content-Type", "application/json")
			resp := httptest.NewRecorder()

			router.ServeHTTP(resp, req)

			if resp.Code != tc.wantStatus {
				t.Fatalf("status = %d, want %d; body=%s", resp.Code, tc.wantStatus, resp.Body.String())
			}
			if fakeDM.deleteCalls != tc.wantDeleteCalls {
				t.Fatalf("DeleteUser calls = %d, want %d", fakeDM.deleteCalls, tc.wantDeleteCalls)
			}
		})
	}
}

func TestAuthGetUserRoute(t *testing.T) {
	gin.SetMode(gin.TestMode)

	tests := []struct {
		name          string
		pathUserID    string
		findUserByID  func(id string) (*database_layer.User, error)
		wantStatus    int
		wantFindCalls int
		wantContains  string
	}{
		{
			name:       "returns user profile",
			pathUserID: "u-123",
			findUserByID: func(id string) (*database_layer.User, error) {
				if id != "u-123" {
					t.Fatalf("FindUserByID id = %q, want %q", id, "u-123")
				}
				user := database_layer.NewUser("u-123", "alice", "alice@example.com", "secret")
				return &user, nil
			},
			wantStatus:    http.StatusOK,
			wantFindCalls: 1,
			wantContains:  `"user_id":"u-123"`,
		},
		{
			name:       "returns not found for unknown user",
			pathUserID: "missing",
			findUserByID: func(id string) (*database_layer.User, error) {
				return nil, nil
			},
			wantStatus:    http.StatusNotFound,
			wantFindCalls: 1,
			wantContains:  `"error":"user not found"`,
		},
		{
			name:       "maps lookup errors",
			pathUserID: "u-500",
			findUserByID: func(id string) (*database_layer.User, error) {
				return nil, errors.New("db offline")
			},
			wantStatus:    http.StatusInternalServerError,
			wantFindCalls: 1,
			wantContains:  `"error":"failed to load user profile"`,
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			fakeDM := &fakeAuthDataManager{findUserByIDFn: tc.findUserByID}

			router := gin.New()
			api := router.Group("/api/v1")
			RegisterAuthRoutes(api, fakeDM, &EventLogger{})

			req := httptest.NewRequest(http.MethodGet, "/api/v1/auth/users/"+tc.pathUserID, nil)
			resp := httptest.NewRecorder()

			router.ServeHTTP(resp, req)

			if resp.Code != tc.wantStatus {
				t.Fatalf("status = %d, want %d; body=%s", resp.Code, tc.wantStatus, resp.Body.String())
			}
			if fakeDM.findCalls != tc.wantFindCalls {
				t.Fatalf("FindUserByID calls = %d, want %d", fakeDM.findCalls, tc.wantFindCalls)
			}
			if tc.wantContains != "" && !bytes.Contains(resp.Body.Bytes(), []byte(tc.wantContains)) {
				t.Fatalf("response body = %s, expected substring %q", resp.Body.String(), tc.wantContains)
			}
		})
	}
}

func TestAuthRegisterLookupDeleteIntegration_UsesDevDB(t *testing.T) {
	gin.SetMode(gin.TestMode)

	// Keep DB filename aligned with dev runtime naming while remaining test-isolated.
	dbPath := filepath.Join(t.TempDir(), "dev.db")
	mgr, err := database_layer.NewSqliteDataManager(dbPath)
	if err != nil {
		t.Fatalf("NewSqliteDataManager() error = %v", err)
	}

	router := gin.New()
	api := router.Group("/api/v1")
	RegisterAuthRoutes(api, mgr, &EventLogger{})

	registerBody := map[string]string{
		"user_id":  "u-int-1",
		"username": "integration-user",
		"email":    "integration@example.com",
		"password": "pw-int-1",
	}
	registerPayload, err := json.Marshal(registerBody)
	if err != nil {
		t.Fatalf("json.Marshal(registerBody) error = %v", err)
	}

	registerReq := httptest.NewRequest(http.MethodPost, "/api/v1/auth/register", bytes.NewReader(registerPayload))
	registerReq.Header.Set("Content-Type", "application/json")
	registerResp := httptest.NewRecorder()
	router.ServeHTTP(registerResp, registerReq)
	if registerResp.Code != http.StatusCreated {
		t.Fatalf("register status = %d, want %d; body=%s", registerResp.Code, http.StatusCreated, registerResp.Body.String())
	}

	lookupReq := httptest.NewRequest(http.MethodGet, "/api/v1/auth/users/u-int-1", nil)
	lookupResp := httptest.NewRecorder()
	router.ServeHTTP(lookupResp, lookupReq)
	if lookupResp.Code != http.StatusOK {
		t.Fatalf("lookup status = %d, want %d; body=%s", lookupResp.Code, http.StatusOK, lookupResp.Body.String())
	}

	var lookupBody AuthResponse
	if err := json.Unmarshal(lookupResp.Body.Bytes(), &lookupBody); err != nil {
		t.Fatalf("json.Unmarshal(lookup) error = %v", err)
	}
	if lookupBody.UserID != "u-int-1" || lookupBody.Username != "integration-user" || lookupBody.Email != "integration@example.com" {
		t.Fatalf("lookup response = %#v, want user_id=%q username=%q email=%q", lookupBody, "u-int-1", "integration-user", "integration@example.com")
	}

	deletePayload, err := json.Marshal(map[string]string{"password": "pw-int-1"})
	if err != nil {
		t.Fatalf("json.Marshal(delete body) error = %v", err)
	}
	deleteReq := httptest.NewRequest(http.MethodDelete, "/api/v1/auth/users/u-int-1", bytes.NewReader(deletePayload))
	deleteReq.Header.Set("Content-Type", "application/json")
	deleteResp := httptest.NewRecorder()
	router.ServeHTTP(deleteResp, deleteReq)
	if deleteResp.Code != http.StatusOK {
		t.Fatalf("delete status = %d, want %d; body=%s", deleteResp.Code, http.StatusOK, deleteResp.Body.String())
	}

	lookupAfterDeleteReq := httptest.NewRequest(http.MethodGet, "/api/v1/auth/users/u-int-1", nil)
	lookupAfterDeleteResp := httptest.NewRecorder()
	router.ServeHTTP(lookupAfterDeleteResp, lookupAfterDeleteReq)
	if lookupAfterDeleteResp.Code != http.StatusNotFound {
		t.Fatalf("lookup-after-delete status = %d, want %d; body=%s", lookupAfterDeleteResp.Code, http.StatusNotFound, lookupAfterDeleteResp.Body.String())
	}
}
