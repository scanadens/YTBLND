# Frontend Client Integration Guide (`ytblnd-backend`)

## 1. Base Configuration
- Base URL: `http://localhost:8080`
- API prefix: `/api/v1`
- JSON header for REST requests: `Content-Type: application/json`
- Current auth model: no JWT/session token required yet

## 2. Health Check
### `GET /ping`
Behavior summary: liveness probe only. No DB access or auth checks.

**Success (200)**
```json
{
  "message": "pong"
}
```

## 3. Auth Endpoints
### `POST /api/v1/auth/register`
Creates a new user account.

Behavior summary: inserts a new `users` row keyed by `user_id`; duplicate IDs are rejected.

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

Behavior summary: validates plaintext password, then loads and returns the stored profile for that user.

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

---

### `GET /api/v1/auth/users/:userID`
Fetches one user profile directly by user ID.

Behavior summary: performs a read-only lookup in `users` and returns identity fields needed by the client without requiring indirect lookup flows through blend/chat endpoints.

**Success (200)**
```json
{
  "user_id": "u1",
  "username": "alice",
  "email": "alice@example.com"
}
```

**Errors**
- `400` missing `userID` path param
- `404` user not found
- `500` server error while loading profile

---

### `DELETE /api/v1/auth/users/:userID`
Deletes a user account after password re-authentication.

This endpoint performs transactional cascade cleanup to avoid orphaned rows and inconsistent state.

**Request body**
```json
{
  "password": "pw123"
}
```

**Success (200)**
```json
{
  "status": "deleted",
  "user_id": "u1"
}
```

**Errors**
- `400` invalid request body or missing `userID` path param
- `401` invalid credentials
- `500` server error during deletion (including not-found or other delete failures)

**Cascade delete behavior (summary)**
- removes user row
- removes user watch-later rows
- removes user blend/chat memberships
- removes creator-owned blend and chat artifacts
- removes chat messages sent by that user
- removes blends that become empty after membership cleanup

## 4. Watch-Later Endpoints
### `POST /api/v1/users/:userID/watch-later`
Replaces the full watch-later list for a user.

Behavior summary: transactional full replace (`DELETE` existing + ordered reinsert). Existing entries are overwritten, not merged.

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

Behavior summary: returns videos in persisted order (`position ASC`) with stored metadata fields.

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

Behavior summary: upserts blend metadata (including optional title), replaces participant membership, ensures one chat room exists for the blend, and syncs room members to participants.

**Request body**
```json
{
  "blend_id": "b1",
  "title": "Roadtrip Mix",
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

Title note: `title` is optional on create/update but persisted in the backend `blends.title` column and returned by blend-listing endpoints.

---

### `GET /api/v1/users/:userID/blend`
Gets the latest blend ID associated with a user.

Behavior summary: returns the most recently created blend containing that user (ordered by `created_at DESC`).

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

Behavior summary: reads current participant membership for the blend as persisted in `blend_participants`.

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

Behavior summary: resolves the blend's attached chat room and returns that room's current member set.

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

---

### `GET /api/v1/users/:userID/blends`
Returns all blends the user currently participates in.

Behavior summary: returns a list of blend summaries (blend ID + title) for frontend blend-picker/navigation views.

**Success (200)**
```json
{
  "user_id": "u2",
  "blends": [
    { "blend_id": "b1", "title": "Roadtrip Mix" },
    { "blend_id": "b2", "title": "Focus Session" }
  ]
}
```

**Errors**
- `400` missing/invalid `userID` path param
- `500` server error

---

### `POST /api/v1/blends/:blendID/leave`
Removes one user from a blend and its linked chat room membership.

**Request body**
```json
{
  "user_id": "u2"
}
```

**Success (200)**
```json
{
  "blend_id": "b1",
  "user_id": "u2",
  "status": "left"
}
```

**Errors**
- `400` invalid payload or missing `blendID`
- `500` server error

## 6. WebSocket Chat
### Chat History
`GET /api/v1/blends/:blendID/chat-history?user_id=:userID`

Behavior summary: guarded by the same chat-room membership checks as websocket connect; returns up to the latest 100 persisted messages oldest-to-newest.

**Success (200)**
```json
{
  "blend_id": "b1",
  "chat_room_id": "b1",
  "messages": [
    {
      "type": "chat_message",
      "room_id": "b1",
      "sender_id": "u2",
      "content": "hello everyone",
      "sent_at": "2026-03-26T15:04:05Z"
    }
  ]
}
```

**Errors**
- `400` missing `blendID` path param or `user_id` query param
- `403` user is not a member of the chat room
- `404` chat room missing for blend
- `500` server error

### Connect
`GET ws://localhost:8080/api/v1/ws/chats/:blendID?user_id=:userID`

Behavior summary: upgrade is allowed only for users who are active members of the blend's chat room.

**Example**
`ws://localhost:8080/api/v1/ws/chats/b1?user_id=u2`

### Server validation before upgrade
- Blend must have a chatroom (`404` otherwise)
- `user_id` must be a member of that blend chatroom (`403` otherwise)
- Required path/query params must exist (`400` otherwise)

Validation flow summary: path/query -> chat room lookup by blend -> membership check -> websocket upgrade.

### Client outbound message (minimum required)
```json
{
  "content": "hello everyone"
}
```

Outbound behavior summary: empty/whitespace-only messages are ignored; valid messages are timestamped and broadcast room-scoped.

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

Broadcast behavior summary: messages are only delivered to clients connected to the same `room_id`.

### WS client requirements
- reconnect strategy on disconnect
- show forbidden/not-found errors on failed upgrade
- ignore empty message submissions
- append incoming messages by `room_id`
- use local user identity for `user_id` query param

Persistence note: chat messages are persisted in SQLite and can be read via `GET /api/v1/blends/:blendID/chat-history?user_id=...`.

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
- no JWT-bound websocket auth yet (membership check only)
- add CORS middleware if frontend runs on a different origin