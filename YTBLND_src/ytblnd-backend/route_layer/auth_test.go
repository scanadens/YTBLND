package routelayer

import (
	"bytes"
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/gin-gonic/gin"

	database_layer "ytblnd-backend/database_layer"
)

type fakeAuthDataManager struct {
	validatePasswordFn func(id, password string) bool
	deleteUserFn       func(userID, password string) error

	validateCalls int
	deleteCalls   int
	lastDeleteID  string
	lastDeletePw  string
}

func (f *fakeAuthDataManager) CreateUser(user database_layer.User) error { return nil }

func (f *fakeAuthDataManager) FindUserByID(id string) (*database_layer.User, error) {
	return nil, nil
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
