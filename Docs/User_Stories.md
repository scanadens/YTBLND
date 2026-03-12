# User Stories

<aside>
<img src="https://www.notion.so/icons/brain_purple.svg" alt="https://www.notion.so/icons/brain_purple.svg" width="40px" />

***Contributors***: Jasmine Kumar, Xavier Lusetti, Shamar Pennant

</aside>

This document contains our user stories for YT BLND based on our requirements documentation.

Visit [this page](https://www.notion.so/2f712276ddb4805d9856c9286b48b4f3?pvs=21) for a graphical view. 

# Required

## Save Data Across Sessions:

Story Points: 3

### Story:

As a user, I can save my imported data across sessions so that I don't need to re-upload.

### Acceptance Tests:

- Data persists after sign-out/sign-in
- Can view previously imported statistics
- Option to refresh/update data

## Block Blend Tags:

Story Points: 5

### Story:

As a user, I can block specific tags from appearing in my blend so that I can filter unwanted content

### Acceptance Tests:

- Can add tags to block list
- Blocked videos don't appear in feed
- Can remove tags from block list
- Visual feedback when blocking

## View Blend:

Story Points: 13

### Story:

As a user, I can view my blended feed so that I see content from multiple sources in one place

### Acceptance Tests:

- Displays blended video recommendations
- Shows source indicators (which friend contributed)
- Pagination or infinite scroll
- Videos are clickable/open in YouTube

## Select User(s) to Blend With:

Story Points: 5

### Story:

As a user, I can select 1-3 friends to blend with so that I can create a combined feed.

### Acceptance Tests:

- Display list of available friends (initially by username/email)
- Can select/deselect friends
- Visual confirmation of selection
- Maximum 3 friends enforced

## Sign In Securely:

Story Points: 2

### Story:

As a registered user, I can sign in securely so that my data remains private.

### Acceptance Tests:

- Correct credentials grant access
- Incorrect credentials are rejected
- Session times out after inactivity

## Sign Out Securely:

Story Points: 1

### Story:

As a signed-in user, I can sign out so that my account remains secure on shared devices.

### Acceptance Tests:

- Session is terminated on sign-out
- User cannot access protected pages after sign-out

## Delete User Account:

Story Points: 3

### Story:

As an account holder, I can delete my account so that my data is permanently removed.

### Acceptance Tests:

- All user data is deleted from database
- Confirmation dialog prevents accidental deletion
- Blend relationships are cleaned up

## Create User Account:

Story Points: 3

### Story:

As a new user, I can create an account with email and password so that I can access personalized blend features.

### Acceptance Tests:

- User can enter email/password and receive confirmation
- Duplicate emails are rejected
- Password meets security requirements
- Account data persists

## Upload Data:

Story Points: 8

### Story:

As a user, I can upload my YouTube data via Dropbox file so that the system can analyze my viewing preferences without API access.

### Acceptance Tests:

- Accepts YouTube Takeout JSON/CSV format
- Validates file structure
- Parses watch history and subscriptions
- Shows upload progress and confirmation

# Optional

## Toggle Shorts:

Story Points: 3

### Story:

As a user, I can toggle YouTube Shorts/Reels inclusion so that I can focus on longer content.

### Acceptance Tests:

- Toggle shows "Include Shorts" option
- When off, videos under 60 seconds filtered
- Setting persists per blend
- Visual indication of filter status

## Have Chatroom Moderation Privileges:

Story Points: 3

### Story:

As a moderator, I have complete control over my chatroom.

### Acceptance Tests:

- Creating a chatroom gives me moderator privileges
- As moderator I may assign moderator privileges to another
- As moderator if I may leave and moderator privileges will be assigned to another
- As moderator I have chatroom control to add, remove, members

## Create a Blend Chat with 2-8 People:

Story Points: 13

### Story:

As a user, I can create a blend chat with 2-8 people so that we can discuss blended content

### Acceptance Tests:

- Create chat room from blend
- Invite blend participants
- Real-time messaging
- Chat persists for duration of blend

## Give Access to YouTube Data (Oauth):

Story Points: 8

### Story:

As a user, I can connect my YouTube account via OAuth so that my data stays current without manual uploads

### Acceptance Tests:

- YouTube OAuth flow works
- Can revoke access
- Data auto-updates periodically
- Handles API rate limits gracefully

## Add a Friend:

Story Points: 5

### Story:

As a user, I can add friends by username or email so that I can create blends with them.

### Acceptance Tests:

- Search for users by username/email
- Send/receive friend requests
- Accept/reject requests
- View friend list

## Remove a Friend:

Story Points: 1

### Story:

As a user, I can remove friends by username.

### Acceptance Tests:

- Search for users by username/email
- Remove a selected friend

# Wish list

## Plugin Integration:

Story Points: 8

### Story:

As a *user*, I can *use the app as a plugin* so that *I can seamlessly message and view videos while doing other things in my browser*

### Acceptance Tests:

- plugin installation (given the user is on a chrome web store or equivalent, when they add it to the browser, the plugin installs and appears in the extension list)
- plugin login flow (following installation, when the icon is clicked the user is prompted to sign in with their YT BLND account. where a successful login will persist across browser sessions)
- detecting YouTube pages (given the user is logged in to their YT BLND account, navigating to a YouTube page will activate the plugin. displaying options such as “add video to blend” or “view chat rooms”)
- adding a video from a YouTube page to a blend (assuming the user is on a YouTube video, when “Add to blend” is clicked in the plugin, a list of chatrooms appear. where selecting a chatroom successful adds the video to that blends candidate pool)
- error handling (given the user is offline or the backend is unreachable, if the user attempts using plugin features, the plugin will clearly indicate an error message
- privacy and permissions (given the plugin is installed, when the user first opens it, the plugin will request only minimal permissions (such as reading the contents of a browser page, and accessing youtube tabs. The user able to deny permissions and/or uninstall/disable the plugin cleanly)

## Export Blend:

Story Points: 5

### Story:

As a user, I can export my blend as a YouTube playlist so that I can watch on other devices.

### Acceptance Tests:

- Generate YouTube playlist link
- Copy to clipboard functionality
- Limit to 50 videos (YouTube limit)

## View Compatibility:

Story Points: 5

### Story:

As a user, I can see a "compatibility score" with friends so that I understand our viewing overlap.

### Acceptance Tests:

- Display percentage overlap
- Show top shared interests
- Update score as data changes

## Adjust Blend Ratio:

Story Points: 8

### Story:

As a user, I can adjust the "blend ratio" with sliders so that I can control each friend's influence on recommendations.

### Acceptance Tests:

- Sliders for each friend (0-100%)
- Visual preview of ratio changes
- Ratio affects next batch of recommendations
- Default is equal distribution

## Discover New Videos:

Story Points: 13

### Story:

As a user, I can discover new videos that weren't in any original feeds so that I find content aligned with our combined interests.

### Acceptance Tests:

- Blend includes "discovery" videos
- Discovery videos have "why recommended" explanation
- Can rate discovery relevance
- Discovery improves with feedback

## Select a Graphical Theme:

Story Points: 5

### Story:

As a user, I can customize my graphical interface among given options as I desire.

### Acceptance Tests:

- Theme selector in settings
- Changes apply immediately
- Theme persists across sessions
- At least 3 theme options (light, dark, custom)