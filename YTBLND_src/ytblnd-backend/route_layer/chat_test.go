package routelayer

import (
	"testing"
	"time"
)

func TestChatHub_BroadcastIsRoomScoped(t *testing.T) {
	hub := NewChatHub(nil, nil)
	go hub.Run()

	roomOneClient := &ChatClient{hub: hub, send: make(chan ChatMessage, 1), roomID: "room-one", userID: "u-1"}
	roomTwoClient := &ChatClient{hub: hub, send: make(chan ChatMessage, 1), roomID: "room-two", userID: "u-2"}

	hub.register <- roomOneClient
	hub.register <- roomTwoClient

	hub.broadcast <- broadcastEvent{
		roomID: "room-one",
		message: ChatMessage{
			Type:     "chat_message",
			RoomID:   "room-one",
			SenderID: "u-1",
			Content:  "hello",
		},
	}

	select {
	case msg := <-roomOneClient.send:
		if msg.RoomID != "room-one" {
			t.Fatalf("roomOneClient message room = %q, want %q", msg.RoomID, "room-one")
		}
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for room-one broadcast")
	}

	select {
	case msg := <-roomTwoClient.send:
		t.Fatalf("roomTwoClient unexpectedly received message: %#v", msg)
	case <-time.After(200 * time.Millisecond):
	}

	hub.unregister <- roomOneClient
	hub.unregister <- roomTwoClient
}
