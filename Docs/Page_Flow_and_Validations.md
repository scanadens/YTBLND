# YTBLND — Page Flow & Validations

## Pages

| Enum | Panel Class | Purpose |
|------|-------------|---------|
| `LOGIN` (0) | `LoginPanel` | Sign In / Register |
| `DATA_INSTRUCTIONS` (1) | `DataInstructionsPanel` | Google Takeout CSV upload guide |
| `BLEND_CREATION` (2) | `BlendCreationPanel` | Add participants, create blend |
| `HOME` (3) | `BlendFeedPanel` (inside `MainFrame::BuildHomePage`) | 3×2 video grid feed |
| `USER` (4) | `UserPanel` | Account info + logout |
| `SETTINGS` (5) | *(stub)* | Settings — coming soon |
| `BLEND_CHAT` (6) | `BlendChatPanel` | In-session chat for active blend |

---

## Full Page Flow Diagram

```
App Launch
    │
    ▼
┌─────────┐
│  LOGIN  │◄──────────────────────────────────────────────────────┐
└────┬────┘                                                        │
     │                                                             │
     │  Sign In / Register                                         │
     ▼                                                             │
  [Validation: credentials / register fields]                      │
     │                                                             │
     ├── No blend, no data ──────────► DATA_INSTRUCTIONS           │
     │                                      │                      │
     ├── No blend, has data ─────────► BLEND_CREATION              │
     │                                      │                      │
     └── Existing blend found ──────► HOME (feed loaded)           │
                                                                   │
                                                                   │
DATA_INSTRUCTIONS                                                  │
     │                                                             │
     ├── Browse CSV → upload → BLEND_CREATION                      │
     │   (if already a blend participant → re-generates blend       │
     │    and navigates to HOME instead)                            │
     └── Skip → BLEND_CREATION                                     │
                                                                   │
                                                                   │
BLEND_CREATION                                                     │
     │  Back button ─────────────────────────────────────────────► │ (LOGIN)
     │                                                             │
     │  Add users → Create Blend                                   │
     ▼                                                             │
  [Validation: ≥2 users, data checks]                              │
     │                                                             │
     ├── All users have data ─────────────────────────► HOME       │
     ├── Some users missing data ─── warning dialog ──► HOME       │
     └── No users have data ──────── error dialog ──── (stay)      │
                                                                   │
                                                                   │
HOME  ◄──────── feedPanel->LoadFromBlend() on every arrival        │
     │                                                             │
     ├── [Settings button] ──────────────────────► SETTINGS        │
     │                                               │             │
     │                                          Back │             │
     │                                               ▼             │
     │                                             HOME            │
     │                                                             │
     ├── [User button] ──────────────────────────► USER            │
     │                                               │             │
     │                                      Logout ──┼─────────────┘
     │                                      Back  ──► HOME
     │
     └── [Blend button]
              │
              ├── Active blend exists ──────────► BLEND_CHAT
              │                                       │
              │                                  Back ▼
              │                                     HOME
              │
              └── No active blend ──────────────► BLEND_CREATION
```

---

## Validations by Page

### LOGIN — Sign In tab

| Field | Rule | Error shown |
|-------|------|-------------|
| Username | Must not be empty | "Please enter your username and password." |
| Password | Must not be empty | "Please enter your username and password." |
| Credentials | Must match a stored user record via `SqliteDataManager::validatePassword` | "Incorrect username or password." |

**On success:** calls `ProceedAfterLogin()` — see post-login routing below.

---

### LOGIN — Register tab

| Field | Rule | Error shown |
|-------|------|-------------|
| Username | Must not be empty | "Username and password are required." |
| Password | Must not be empty | "Username and password are required." |
| Password | Must be ≥ 6 characters | "Password must be at least 6 characters." |
| Confirm password | Must match Password field | "Passwords do not match." |
| Username | Must not already exist in DB (`createUser` returns false on duplicate) | "Username already taken. Choose another." |

**On success:** calls `ProceedAfterLogin()`.

---

### Post-Login Routing (`LoginPanel::ProceedAfterLogin`)

Evaluated in order after every successful sign-in or registration:

```
1. appState.getActiveBlend() != nullptr
       └── handleLogin re-generated a blend from persisted data
       └── Navigate to: HOME

2. user->getYouTubeData().getWatchLaterVideos().empty() == true
       └── User has no Watch Later data uploaded yet
       └── If appState.takePendingBlendMessage() is non-empty:
               └── Show info dialog: "You have been added to a blend.
                   Upload your Watch Later data to contribute your videos to it."
       └── Navigate to: DATA_INSTRUCTIONS

3. Otherwise (has data, no blend yet)
       └── Navigate to: BLEND_CREATION
```

---

### DATA_INSTRUCTIONS

| Action | Behaviour |
|--------|-----------|
| Browse for CSV | Opens file picker (CSV only). On confirm: dispatches `uploadData` event → `AppController::handleUploadData` parses and saves to DB. If the user is already a blend participant the algorithm re-runs and `activeBlend` is updated. Navigates to `BLEND_CREATION` (or `HOME` if blend was re-generated). |
| Skip for now | Navigates directly to `BLEND_CREATION` without uploading anything. |

---

### BLEND_CREATION

**On Reload (every arrival):**
- The logged-in user's username is pre-populated in the list (cannot be removed).
- "Create Blend" button starts disabled.

**Add button:**

| Rule | Error shown |
|------|-------------|
| Max 8 users total | "Maximum 8 users reached." |
| Username must not be empty | *(dialog simply won't confirm)* |
| Username must not already be in the list | "User \"X\" is already in the list." |

> **Note:** User existence in the DB is not validated at add-time (optimistic add). The DB check happens at blend-creation time.

**Create Blend button:** Enabled when `m_addedUsers.size() >= 2` (current user counts as one, so adding one other person enables it).

**On Create:**

`handleCreateBlend` loads each participant from the DB and checks for Watch Later data:

| Scenario | Behaviour |
|----------|-----------|
| User ID not found in DB | Added to `usersWithoutData` list |
| User found but `user_watch_later` table empty | Added to `usersWithoutData` list |
| User found with data | Added to `participants` for algorithm |
| All participants missing data | Error dialog: "No blend could be created. Please ensure at least one user has uploaded their data." — stays on BLEND_CREATION |
| Some participants missing data | Warning dialog listing missing users + "Their videos were not included." — navigates to HOME |
| All participants have data | Navigates to HOME silently |

**Blend persistence (on successful create):**
- Blend saved to `blends` table with `creator_id` = logged-in user.
- All requested participant IDs (including those without data) saved to `blend_participants` — so they will find this blend on their next login even before uploading data.

**Back button:** Navigates to `LOGIN` (not HOME — HOME would show an empty feed).

---

### HOME

**On arrival:** `feedPanel->LoadFromBlend()` is called automatically.

| State | Feed display |
|-------|-------------|
| `activeBlend` is set with videos | Shows 3×2 grid of video cards |
| `activeBlend` is null or empty | All 6 cards show placeholder state |

**Refresh button:** Advances to the next page of 6 videos (`NextPage()` wraps around).

**Blend button:**
- Active blend exists → `BLEND_CHAT`
- No active blend → `BLEND_CREATION`

---

### USER

| Action | Behaviour |
|--------|-----------|
| Logout | Shows `ConfirmationDialog`. On confirm: dispatches `logout` → `AppController::handleLogout` clears all `AppState` (user, blend, chat rooms, session users) → navigates to `LOGIN`. |
| Back | Navigates to `HOME`. |

---

### SETTINGS *(stub)*

| Action | Behaviour |
|--------|-----------|
| Back | Navigates to `HOME`. |

---

### BLEND_CHAT

**On Reload (every arrival):** Refreshes message list from `AppState::getActiveChatRoom()`.

| Action | Behaviour |
|--------|-----------|
| Send (button or Enter key) | Dispatches `sendMessage` with `{userID, text}`. *(Handler is currently a stub — message display not yet wired.)* |
| Back | Navigates to `HOME`. |

---

## AppController Event → Handler Summary

| Event dispatched | Handler | Key side effects |
|-----------------|---------|-----------------|
| `"register"` | `handleRegister` | Creates user in DB, sets `currentUser` |
| `"login"` | `handleLogin` | Validates credentials, loads Watch Later from DB, loads + re-generates blend if one exists, sets `pendingBlendMessage` if user has no data |
| `"logout"` | `handleLogout` | Clears all `AppState`, frees `currentUser` |
| `"uploadData"` | `handleUploadData` | Parses CSV, saves to `user_watch_later`, updates `currentUser` YouTubeData, re-runs blend if user is a participant |
| `"createBlend"` | `handleCreateBlend` | Loads participant data from DB, runs `RandomBlendAlgorithm`, saves blend + participants to DB, sets `activeBlend`, sets `usersWithoutData` |
| `"playVideo"` | `handlePlayVideo` | *(stub)* |
| `"refresh"` | `handleRefresh` | *(stub)* |
| `"openChat"` | `handleOpenChat` | *(stub)* |
| `"sendMessage"` | `handleSendMessage` | *(stub)* |

---

## Database Tables (SQLite — `ytblnd.db`)

```sql
users              (user_id PK, username, email, password)
user_watch_later   (user_id + video_id PK, title, position)
blends             (blend_id PK, creator_id, algorithm, created_at)
blend_participants (blend_id + user_id PK)
```

**Persistence rules:**
- `user_watch_later` rows for a user are deleted and re-inserted on every CSV upload (clean re-upload).
- `blend_participants` rows are deleted and re-inserted on every `saveBlend` call (supports future participant management).
- Blend videos are **not** stored — the algorithm is re-run from `user_watch_later` on every login.
