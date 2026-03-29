# Frontend Client Integration Guide (`ytblnd-backend`)

## 1. Base Configuration
- Base URL: `http://localhost:8080`
- API prefix: `/api/v1`
- JSON header for REST requests: `Content-Type: application/json`
- Current auth model: no JWT/session token required yet

## 2. Health Check
### `GET /ping`
**Success (200)**
```json
{
  "message": "pong"
}
```

## 3. Auth Endpoints
### `POST /api/v1/auth/register`
Creates a new user account.

**Request body**
```json
{
  "user_id": "u1",
  "username": "alice",
  "email": "alice@example.com",
  "password": "pw123"
}
```

**Success (201)**
```json
{
  "user_id": "u1",
  "username": "alice",
  "email": "alice@example.com"
}
```

**Errors**
- `400` invalid JSON or missing required fields
- `409` duplicate user / create failure

---

### `POST /api/v1/auth/login`
Authenticates a user.

**Request body**
```json
{
  "user_id": "u1",
  "password": "pw123"
}
```

**Success (200)**
```json
{
  "user_id": "u1",
  "username": "alice",
  "email": "alice@example.com"
}
```

**Errors**
- `400` invalid request
- `401` invalid credentials
- `500` server error

## 4. Watch-Later Endpoints
### `POST /api/v1/users/:userID/watch-later`
Replaces the full watch-later list for a user.

**Request body**
```json
{
  "videos": [
    { "video_id": "vid1", "title": "Video One" },
    { "video_id": "vid2", "title": "Video Two" }
  ]
}
```

**Success (200)**
```json
{
  "user_id": "u1",
  "saved_count": 2,
  "watch_later_ok": true
}
```

**Errors**
- `400` bad path/body
- `500` server error

---

### `GET /api/v1/users/:userID/watch-later`
Loads a user's current watch-later list.

**Success (200)**
```json
{
  "user_id": "u1",
  "videos": [
    { "video_id": "vid1", "title": "Video One" },
    { "video_id": "vid2", "title": "Video Two" }
  ]
}
```

## 5. Blend + Chatroom Endpoints
### `POST /api/v1/blends`
Creates or updates a blend and auto-attaches a chatroom.

**Request body**
```json
{
  "blend_id": "b1",
  "creator_id": "u1",
  "algorithm": "round_robin",
  "participants": ["u1", "u2", "u3"]
}
```

**Success (200)**
```json
{
  "blend_id": "b1",
  "chat_room_id": "b1",
  "status": "saved"
}
```

---

### `GET /api/v1/users/:userID/blend`
Gets the latest blend ID associated with a user.

**Success (200)**
```json
{
  "user_id": "u2",
  "blend_id": "b1"
}
```

**Errors**
- `404` no blend for user

---

### `GET /api/v1/blends/:blendID/participants`
Returns all blend participants.

**Success (200)**
```json
{
  "blend_id": "b1",
  "participants": ["u1", "u2", "u3"]
}
```

---

### `GET /api/v1/blends/:blendID/chatroom`
Returns the chatroom attached to a blend and all members.

**Success (200)**
```json
{
  "blend_id": "b1",
  "chat_room_id": "b1",
  "member_users": ["u1", "u2", "u3"]
}
```

**Errors**
- `404` chatroom not found for blend

## 6. WebSocket Chat
### Connect
`GET ws://localhost:8080/api/v1/ws/chats/:blendID?user_id=:userID`

**Example**
`ws://localhost:8080/api/v1/ws/chats/b1?user_id=u2`

### Server validation before upgrade
- Blend must have a chatroom (`404` otherwise)
- `user_id` must be a member of that blend chatroom (`403` otherwise)
- Required path/query params must exist (`400` otherwise)

### Client outbound message (minimum required)
```json
{
  "content": "hello everyone"
}
```

### Server outbound message shape
```json
{
  "type": "chat_message",
  "room_id": "b1",
  "sender_id": "u2",
  "content": "hello everyone",
  "sent_at": "2026-03-26T15:04:05Z"
}
```

### WS client requirements
- reconnect strategy on disconnect
- show forbidden/not-found errors on failed upgrade
- ignore empty message submissions
- append incoming messages by `room_id`
- use local user identity for `user_id` query param

## 7. Recommended Frontend Interaction Flow
1. Register or login user.
2. Save watch-later for users as needed.
3. Create blend with the participating users.
4. Read returned `chat_room_id` or verify via `GET /blends/:blendID/chatroom`.
5. Connect websocket with blend ID and user ID.
6. Send and render realtime messages.

## 8. Error-Handling Map
- `400`: invalid payload, path, or query
- `401`: invalid auth credentials
- `403`: user not allowed in blend chatroom
- `404`: blend or chatroom not found
- `409`: duplicate user conflict
- `500`: backend error

## 9. Current Backend Constraints
- no JWT or session enforcement yet
- passwords currently plaintext for dev-stage behavior
- no persisted chat history endpoint yet, only realtime broadcast
- add CORS middleware if frontend runs on a different origin
