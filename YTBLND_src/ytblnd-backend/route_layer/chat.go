package routelayer

import (
	"log"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"

	database_layer "ytblnd-backend/database_layer"
)

const (
	// writeWait bounds the time allowed for each outbound websocket frame.
	writeWait = 10 * time.Second
	// pongWait bounds the interval before considering a client dead.
	pongWait = 60 * time.Second
	// pingPeriod sends periodic keepalive pings before pongWait expires.
	pingPeriod = (pongWait * 9) / 10
	// maxMessageSize limits inbound payload size per client message.
	maxMessageSize = 4096
)

// websocketUpgrader upgrades HTTP requests to websocket connections.
var websocketUpgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		// Keep permissive for local development.
		return true
	},
}

// ChatMessage is the websocket JSON envelope exchanged with clients.
type ChatMessage struct {
	Type     string `json:"type"`
	RoomID   string `json:"room_id"`
	SenderID string `json:"sender_id"`
	Content  string `json:"content"`
	SentAt   string `json:"sent_at"`
}

// broadcastEvent wraps a room-targeted outbound chat message.
type broadcastEvent struct {
	roomID  string
	message ChatMessage
}

// ChatClient tracks one websocket connection bound to a room/user.
type ChatClient struct {
	hub    *ChatHub
	conn   *websocket.Conn
	send   chan ChatMessage
	roomID string
	userID string
}

// ChatHub keeps room membership and broadcasts events to each room.
type ChatHub struct {
	rooms      map[string]map[*ChatClient]bool
	register   chan *ChatClient
	unregister chan *ChatClient
	broadcast  chan broadcastEvent
	store      database_layer.DataManager
	logger     *EventLogger
}

// NewChatHub creates an in-memory chat hub.
func NewChatHub(store database_layer.DataManager, logger *EventLogger) *ChatHub {
	return &ChatHub{
		rooms:      make(map[string]map[*ChatClient]bool),
		register:   make(chan *ChatClient),
		unregister: make(chan *ChatClient),
		broadcast:  make(chan broadcastEvent),
		store:      store,
		logger:     logger,
	}
}

// chatHistoryResponse returns persisted chat messages for one blend room.
type chatHistoryResponse struct {
	BlendID    string        `json:"blend_id"`
	ChatRoomID string        `json:"chat_room_id"`
	Messages   []ChatMessage `json:"messages"`
}

// Run processes client lifecycle and room broadcasts.
func (h *ChatHub) Run() {
	for {
		select {
		case client := <-h.register:
			if h.rooms[client.roomID] == nil {
				h.rooms[client.roomID] = make(map[*ChatClient]bool)
				h.logger.LogEvent("chat_room_activated", "room_id=%s", client.roomID)
			}
			h.rooms[client.roomID][client] = true
			h.logger.LogEvent("chat_client_connected", "room_id=%s user_id=%s room_clients=%d", client.roomID, client.userID, len(h.rooms[client.roomID]))

		case client := <-h.unregister:
			if clients, ok := h.rooms[client.roomID]; ok {
				if _, present := clients[client]; present {
					delete(clients, client)
					close(client.send)
					h.logger.LogEvent("chat_client_disconnected", "room_id=%s user_id=%s room_clients=%d", client.roomID, client.userID, len(clients))
				}
				if len(clients) == 0 {
					delete(h.rooms, client.roomID)
					h.logger.LogEvent("chat_room_deactivated", "room_id=%s", client.roomID)
				}
			}

		case event := <-h.broadcast:
			if clients, ok := h.rooms[event.roomID]; ok {
				h.logger.LogEvent("chat_message_broadcast", "room_id=%s recipients=%d sender_id=%s content_length=%d", event.roomID, len(clients), event.message.SenderID, len(event.message.Content))
				for client := range clients {
					select {
					case client.send <- event.message:
					default:
						close(client.send)
						delete(clients, client)
					}
				}
			}
		}
	}
}

// RegisterChatRoutes mounts websocket chat routes.
func RegisterChatRoutes(api *gin.RouterGroup, dataManager database_layer.DataManager, hub *ChatHub) {
	api.GET("/blends/:blendID/chat-history", func(c *gin.Context) {
		getChatHistory(c, dataManager, hub.logger)
	})
	api.GET("/ws/chats/:blendID", func(c *gin.Context) {
		serveChatWS(c, dataManager, hub)
	})
}

func authorizeChatRoom(c *gin.Context, dataManager database_layer.DataManager, logger *EventLogger) (string, string, string, bool) {
	blendID := c.Param("blendID")
	userID := c.Query("user_id")
	if blendID == "" || userID == "" {
		logger.LogEvent("chat_connection_rejected", "reason=missing_params blend_id=%s user_id=%s client_ip=%s", blendID, userID, c.ClientIP())
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path param and user_id query param are required"})
		return "", "", "", false
	}

	chatRoomID, err := dataManager.GetChatRoomForBlend(blendID)
	if err != nil {
		logger.LogEvent("chat_connection_failed", "reason=room_lookup blend_id=%s user_id=%s client_ip=%s error=%q", blendID, userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to resolve chat room for blend"})
		return "", "", "", false
	}
	if chatRoomID == "" {
		logger.LogEvent("chat_connection_rejected", "reason=missing_room blend_id=%s user_id=%s client_ip=%s", blendID, userID, c.ClientIP())
		c.JSON(http.StatusNotFound, gin.H{"error": "chat room does not exist for blend"})
		return "", "", "", false
	}

	isMember, err := dataManager.IsChatRoomMember(chatRoomID, userID)
	if err != nil {
		logger.LogEvent("chat_connection_failed", "reason=membership_check blend_id=%s room_id=%s user_id=%s client_ip=%s error=%q", blendID, chatRoomID, userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to validate chat room membership"})
		return "", "", "", false
	}
	if !isMember {
		logger.LogEvent("chat_connection_rejected", "reason=not_member blend_id=%s room_id=%s user_id=%s client_ip=%s", blendID, chatRoomID, userID, c.ClientIP())
		c.JSON(http.StatusForbidden, gin.H{"error": "user is not a member of this chat room"})
		return "", "", "", false
	}

	return blendID, userID, chatRoomID, true
}

func getChatHistory(c *gin.Context, dataManager database_layer.DataManager, logger *EventLogger) {
	blendID, userID, chatRoomID, ok := authorizeChatRoom(c, dataManager, logger)
	if !ok {
		return
	}

	limit := 100
	messages, err := dataManager.LoadChatMessages(chatRoomID, limit)
	if err != nil {
		logger.LogEvent("chat_history_failed", "blend_id=%s room_id=%s user_id=%s client_ip=%s error=%q", blendID, chatRoomID, userID, c.ClientIP(), err.Error())
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load chat history"})
		return
	}

	response := make([]ChatMessage, 0, len(messages))
	for _, message := range messages {
		response = append(response, ChatMessage{
			Type:     "chat_message",
			RoomID:   message.GetRoomID(),
			SenderID: message.GetSenderID(),
			Content:  message.GetContent(),
			SentAt:   message.GetSentAt(),
		})
	}

	logger.LogEvent("chat_history_loaded", "blend_id=%s room_id=%s user_id=%s count=%d client_ip=%s", blendID, chatRoomID, userID, len(response), c.ClientIP())
	c.JSON(http.StatusOK, chatHistoryResponse{
		BlendID:    blendID,
		ChatRoomID: chatRoomID,
		Messages:   response,
	})
}

// serveChatWS validates blend membership, upgrades the connection, and
// registers the client in the in-memory chat hub.
func serveChatWS(c *gin.Context, dataManager database_layer.DataManager, hub *ChatHub) {
	blendID, userID, chatRoomID, ok := authorizeChatRoom(c, dataManager, hub.logger)
	if !ok {
		return
	}

	conn, err := websocketUpgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		log.Printf("chat websocket upgrade failed: %v", err)
		hub.logger.LogEvent("chat_connection_failed", "reason=upgrade blend_id=%s room_id=%s user_id=%s client_ip=%s error=%q", blendID, chatRoomID, userID, c.ClientIP(), err.Error())
		return
	}

	client := &ChatClient{
		hub:    hub,
		conn:   conn,
		send:   make(chan ChatMessage, 256),
		roomID: chatRoomID,
		userID: userID,
	}

	hub.logger.LogEvent("chat_connection_accepted", "blend_id=%s room_id=%s user_id=%s client_ip=%s", blendID, chatRoomID, userID, c.ClientIP())
	client.hub.register <- client

	// Start independent read/write pumps for this websocket client.
	go client.writeLoop()
	go client.readLoop()
}

// readLoop continuously consumes client JSON frames and forwards valid chat
// messages into the room broadcast pipeline.
func (c *ChatClient) readLoop() {
	defer func() {
		c.hub.unregister <- c
		_ = c.conn.Close()
	}()

	c.conn.SetReadLimit(maxMessageSize)
	_ = c.conn.SetReadDeadline(time.Now().Add(pongWait))
	c.conn.SetPongHandler(func(string) error {
		return c.conn.SetReadDeadline(time.Now().Add(pongWait))
	})

	for {
		var msg ChatMessage
		if err := c.conn.ReadJSON(&msg); err != nil {
			c.hub.logger.LogEvent("chat_read_closed", "room_id=%s user_id=%s error=%q", c.roomID, c.userID, err.Error())
			break
		}

		// Ignore empty messages to avoid broadcasting no-op payloads.
		if msg.Content == "" {
			c.hub.logger.LogEvent("chat_message_ignored", "room_id=%s user_id=%s reason=empty_content", c.roomID, c.userID)
			continue
		}

		// Overwrite client-supplied routing/identity fields with trusted values.
		msg.Type = "chat_message"
		msg.RoomID = c.roomID
		msg.SenderID = c.userID
		msg.SentAt = time.Now().UTC().Format(time.RFC3339)

		record := database_layer.NewChatMessageRecord(c.roomID, c.userID, msg.Content, msg.SentAt)
		if err := c.hub.store.SaveChatMessage(c.roomID, record); err != nil {
			c.hub.logger.LogEvent("chat_message_persist_failed", "room_id=%s user_id=%s error=%q", c.roomID, c.userID, err.Error())
			continue
		}
		c.hub.logger.LogEvent("chat_message_persisted", "room_id=%s user_id=%s sent_at=%s", c.roomID, c.userID, msg.SentAt)

		c.hub.broadcast <- broadcastEvent{roomID: c.roomID, message: msg}
	}
}

// writeLoop drains outbound room events to the websocket and sends pings to
// keep idle connections healthy.
func (c *ChatClient) writeLoop() {
	ticker := time.NewTicker(pingPeriod)
	defer func() {
		ticker.Stop()
		_ = c.conn.Close()
	}()

	for {
		select {
		case message, ok := <-c.send:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if !ok {
				_ = c.conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}

			if err := c.conn.WriteJSON(message); err != nil {
				c.hub.logger.LogEvent("chat_write_failed", "room_id=%s user_id=%s error=%q", c.roomID, c.userID, err.Error())
				return
			}

		case <-ticker.C:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				c.hub.logger.LogEvent("chat_ping_failed", "room_id=%s user_id=%s error=%q", c.roomID, c.userID, err.Error())
				return
			}
		}
	}
}
