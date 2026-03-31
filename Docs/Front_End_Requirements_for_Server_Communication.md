# Front End Requirements for Server Communication

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

***Author***: Shamar Pennant

</aside>

## 1. Base Configuration

- **Base origin:** `http://137.220.58.22:8080`
- **REST prefix:** `/api/v1`
- **REST request body:** `application/json`
- **REST response type:** JSON
- **WebSocket endpoint:** `ws://137.220.58.22:8080/api/v1/ws/chats/:blendID?user_id=<userID>`

**Example local REST base:**

```
http://localhost:8080/api/v1
```

**Operational note:** The Chicago server instance is running on **1 GB of memory**, so it cannot handle heavy load or large spikes in concurrent traffic. Keep requests lightweight and avoid unnecessary polling.

---

## 2. Global Rules

These rules apply to all REST and WebSocket interactions.

- Always send valid JSON for REST requests with a body.
- Provide all required path and query parameters.
- Treat all IDs (`user_id`, `blend_id`, `chat_room_id`) as opaque strings.
- Handle errors using HTTP status codes and JSON bodies shaped like:
    
    ```json
    { "error": "..." }
    ```
    
- No authentication tokens, cookies, or JWTs are expected.

### Status Code Guide

- **200 / 201** — success
- **400** — malformed request
- **401** — invalid login credentials
- **403** — user not authorized for the requested resource
- **404** — missing user/blend/chat room context
- **409** — duplicate or conflict (commonly registration)
- **500** — server error

---

## 3. Integration Checklist

Use this as the frontend completion list for first-pass integration.

- [ ]  Health check: `GET /ping` → `{ "message": "pong" }`
- [ ]  Register users: `POST /api/v1/auth/register` with `user_id`, `username`, `password`, optional `email`
- [ ]  Login users: `POST /api/v1/auth/login` with `user_id`, `password`
- [ ]  Save watch-later list: `POST /api/v1/users/:userID/watch-later` with full `videos` array (include channel/thumbnail metadata when available)
- [ ]  Load watch-later list: `GET /api/v1/users/:userID/watch-later`
- [ ]  Create/update blend: `POST /api/v1/blends` with `blend_id`, `creator_id`, `algorithm`, `participants[]`
- [ ]  Persist returned `chat_room_id` as the source of truth
- [ ]  Resolve latest user blend: `GET /api/v1/users/:userID/blend`
- [ ]  Load blend participants: `GET /api/v1/blends/:blendID/participants`
- [ ]  Load chat room details: `GET /api/v1/blends/:blendID/chatroom`
- [ ]  Load chat history: `GET /api/v1/blends/:blendID/chat-history?user_id=<userID>`
- [ ]  Open WebSocket: `GET /api/v1/ws/chats/:blendID?user_id=<userID>`
- [ ]  Send outbound socket messages as `{ "content": "..." }`
- [ ]  Render inbound chat message fields (`sender_id`, `sent_at`, `room_id`) as canonical
- [ ]  On reconnect, re-fetch history to cover gaps

---

## 4. Authentication Flow

### Register

```json
POST /api/v1/auth/register
{
  "user_id": "u1",
  "username": "alice",
  "password": "pw123",
  "email": "alice@example.com" (optional)
}
```

**Success (201):** user object

### Login

```json
POST /api/v1/auth/login
{
  "user_id": "u1",
  "password": "pw123"
}
```

**Success (200):** user object

---

## 5. Watch-Later Endpoints

### Save Watch-Later

**Backward compatibility note:** `channel_id`, `channel_name`, `thumbnail_url`, and `channel_logo_url` are optional. Frontends may continue sending only `video_id` and `title`, but should include the metadata fields whenever available to improve UI quality.

```json
POST /api/v1/users/:userID/watch-later
{
  "videos": [
    {
      "video_id": "vid1",
      "title": "Video One",
      "channel_id": "UC_x5XG1OV2P6uZZ5FSM9Ttw",
      "channel_name": "Google for Developers",
      "thumbnail_url": "https://i.ytimg.com/vi/vid1/hqdefault.jpg",
      "channel_logo_url": "https://yt3.ggpht.com/example1"
    },
    {
      "video_id": "vid2",
      "title": "Video Two",
      "channel_id": "UC-lHJZR3Gqxm24_Vd_AJ5Yw",
      "channel_name": "PewDiePie",
      "thumbnail_url": "https://i.ytimg.com/vi/vid2/hqdefault.jpg",
      "channel_logo_url": "https://yt3.ggpht.com/example2"
    }
  ]
}
```

**Success (200):** `{ "user_id": "u1", "saved_count": 2, "watch_later_ok": true }`

### Load Watch-Later

```
GET /api/v1/users/:userID/watch-later
```

**Success (200):**

```json
{
  "user_id": "u1",
  "videos": [
    {
      "video_id": "vid1",
      "title": "Video One",
      "channel_id": "UC_x5XG1OV2P6uZZ5FSM9Ttw",
      "channel_name": "Google for Developers",
      "thumbnail_url": "https://i.ytimg.com/vi/vid1/hqdefault.jpg",
      "channel_logo_url": "https://yt3.ggpht.com/example1"
    }
  ]
}
```

---

## 6. Blend + Chatroom Endpoints

### Create or Update Blend

```json
POST /api/v1/blends
{
  "blend_id": "b1",
  "creator_id": "u1",
  "algorithm": "round_robin",
  "participants": ["u1", "u2", "u3"]
}
```

**Success (200):** `{ "blend_id": "b1", "chat_room_id": "b1", "status": "saved" }`

### Get User's Latest Blend

```
GET /api/v1/users/:userID/blend
```

**Success (200):** `{ "user_id": "u2", "blend_id": "b1" }`

### Get Blend Participants

```
GET /api/v1/blends/:blendID/participants
```

**Success (200):** `{ "blend_id": "b1", "participants": [...] }`

### Get Chatroom Details

```
GET /api/v1/blends/:blendID/chatroom
```

**Success (200):** `{ "blend_id": "b1", "chat_room_id": "b1", "member_users": [...] }`

---

## 7. Chat: History + Live WebSocket

### Load Chat History

```
GET /api/v1/blends/:blendID/chat-history?user_id=:userID
```

Returns up to 100 messages, oldest → newest.

### Connect WebSocket

```
ws://137.220.58.22:8080/api/v1/ws/chats/:blendID?user_id=:userID
```

### Outbound Message (client → server)

```json
{ "content": "hello" }
```

### Inbound Message (server → client)

```json
{
  "type": "chat_message",
  "room_id": "b1",
  "sender_id": "u2",
  "content": "hello",
  "sent_at": "2026-03-26T15:04:05Z"
}
```

### Client Guardrails

- Use one WebSocket per active blend chat room.
- Close sockets when leaving the chat view.
- Assume chat is in-memory per server process (no cross-instance sync).
- Server ignores client-sent `type`, `room_id`, `sender_id`, `sent_at`.
- Outbound `content` must be non-empty.
- On reconnect, reload history.

---

## 8. Error Handling Map

- **400** — invalid payload, path, or query
- **401** — invalid credentials
- **403** — user not allowed in blend chatroom
- **404** — blend or chatroom not found
- **409** — duplicate user conflict
- **500** — backend error

---

## 9. Backend Constraints

- No JWT or session enforcement yet
- Passwords stored plaintext for development
- Chat history endpoint now implemented (up to 100 messages)
- CORS required if frontend runs on a different origin
- Server instance has **limited memory (1 GB)** — avoid heavy load

---

## 10. Recommended Frontend Flow

1. Register or log in the user.
2. Save watch-later list as needed.
3. Create a blend with participants.
4. Store the returned `chat_room_id`.
5. Connect WebSocket using blend ID + user ID.
6. Send and render real-time messages.
