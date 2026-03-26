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
}

// NewChatHub creates an in-memory chat hub.
func NewChatHub() *ChatHub {
	return &ChatHub{
		rooms:      make(map[string]map[*ChatClient]bool),
		register:   make(chan *ChatClient),
		unregister: make(chan *ChatClient),
		broadcast:  make(chan broadcastEvent),
	}
}

// Run processes client lifecycle and room broadcasts.
func (h *ChatHub) Run() {
	for {
		select {
		case client := <-h.register:
			if h.rooms[client.roomID] == nil {
				h.rooms[client.roomID] = make(map[*ChatClient]bool)
			}
			h.rooms[client.roomID][client] = true

		case client := <-h.unregister:
			if clients, ok := h.rooms[client.roomID]; ok {
				if _, present := clients[client]; present {
					delete(clients, client)
					close(client.send)
				}
				if len(clients) == 0 {
					delete(h.rooms, client.roomID)
				}
			}

		case event := <-h.broadcast:
			if clients, ok := h.rooms[event.roomID]; ok {
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
	api.GET("/ws/chats/:blendID", func(c *gin.Context) {
		serveChatWS(c, dataManager, hub)
	})
}

// serveChatWS validates blend membership, upgrades the connection, and
// registers the client in the in-memory chat hub.
func serveChatWS(c *gin.Context, dataManager database_layer.DataManager, hub *ChatHub) {
	blendID := c.Param("blendID")
	userID := c.Query("user_id")
	if blendID == "" || userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "blendID path param and user_id query param are required"})
		return
	}

	chatRoomID, err := dataManager.GetChatRoomForBlend(blendID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to resolve chat room for blend"})
		return
	}
	if chatRoomID == "" {
		c.JSON(http.StatusNotFound, gin.H{"error": "chat room does not exist for blend"})
		return
	}

	// Enforce authorization using persisted blend-attached chat room membership.
	isMember, err := dataManager.IsChatRoomMember(chatRoomID, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to validate chat room membership"})
		return
	}
	if !isMember {
		c.JSON(http.StatusForbidden, gin.H{"error": "user is not a member of this chat room"})
		return
	}

	conn, err := websocketUpgrader.Upgrade(c.Writer, c.Request, nil)
	if err != nil {
		log.Printf("chat websocket upgrade failed: %v", err)
		return
	}

	client := &ChatClient{
		hub:    hub,
		conn:   conn,
		send:   make(chan ChatMessage, 256),
		roomID: chatRoomID,
		userID: userID,
	}

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
			break
		}

		// Ignore empty messages to avoid broadcasting no-op payloads.
		if msg.Content == "" {
			continue
		}

		// Overwrite client-supplied routing/identity fields with trusted values.
		msg.Type = "chat_message"
		msg.RoomID = c.roomID
		msg.SenderID = c.userID
		msg.SentAt = time.Now().UTC().Format(time.RFC3339)

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
				return
			}

		case <-ticker.C:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				return
			}
		}
	}
}
