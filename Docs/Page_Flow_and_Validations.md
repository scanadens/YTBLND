# YTBLND — Acceptance Test Reference

## 1. Introduction

YTBLND is a desktop application that lets multiple users blend their YouTube watch histories into a shared video feed. Users upload their Google Takeout data, form a blend with friends, and discover videos from each other's libraries — complete with a real-time chatroom for discussion.

This document serves as an acceptance test checklist, organized around a streamlined demo flow for the client-side features and considerations for future extensions.

---

## 2. Demo — Client Side

### Creating Account / Login
- Register a new account with username, password, and confirmation
- Verify validation: empty fields rejected, password must be 6+ characters, mismatched confirmation caught, duplicate usernames rejected
- Log in with valid credentials
- Verify incorrect credentials show an error
- After login, routing directs to the appropriate page based on user state (no data → Data Upload, has data but no blend → Blend Creation, existing blend → Home feed)

### Data Upload
- Navigate to the Data Instructions page (shown automatically for new users with no data)
- Upload a **CSV** file exported from Google Takeout
- Upload an **HTML** watch-history file as an alternative format
- Verify uploaded data is saved and associated with the user in our server

### Blend Creation
- Current user is added to the participant list
- Add other users by username (up to 8 total)
- Verify duplicate and empty usernames are rejected
- Create the blend — verify behavior for:
  - All participants have data → navigates to Home feed
  - Some participants missing data → warning dialog, then Home feed
  - No participants have data → error dialog, stays on page

### The Blend — Feed, Refresh, and Chat
- Home feed displays a 3x2 grid of blended video cards with metadata (title, channel, thumbnail)
- Refresh cycles to the next page of videos 
- Open the chatroom for the active blend
- Send and receive messages in real time (WebSocket-backed)
- Navigate back to the feed from chat

### Delete Account 
- From the User panel, delete account — requires re-authentication with password
- Verify all associated data is cleaned up
- Demonstrates user autonomy over their data and participation

### My Blends
- From the Active Blends panel, navigate to Blend Creation
- Create additional blends with different participants
- Switch between active blends — feed and chatroom update accordingly
- Leave a blend — user is removed from participants and a system message is posted to the chatroom

### Theme Selection
- Choose from our seven unique themes with their own backgrounds from Settings
- Theme applies globally across all panels
- Theme is per-session — does not persist after closing the app

---


## 3. Considerations for Extending the App

### Minor
- Add new members to an already existing blend without recreating it
- Make theme selection persist per-user across sessions instead of per-session
- Toggle YouTube Shorts on or off in the blend feed
- Filter out ads from uploaded watch history data
- General UI cleanup and polish

### Major
- **Blend hierarchy** — distinguish creator from members with role-based privileges; creator can moderate chat and manage blend membership
- **Friends feature** — maintain a friends list and add friends directly to blends
- **OAuth integration** — connect with Google OAuth for seamless data import without manual CSV/HTML export
- **Blend algorithm selection** — offer different algorithms with filters by user, genre, recency, etc.
- **Settings control** — centralize user preferences (theme, algorithm, filters) in a dedicated settings panel
