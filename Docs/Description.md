<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

>*Author*: Shamar Pennant, Yousef Selim
>

</aside>

# Our Pitch

Our product aims to bridge a gap within the YouTube ecosystem that has already been addressed in other digital platforms. Services like Spotify Blend and Instagram Blend allow users to merge their preferences and discover content collaboratively. YouTube, however, remains a highly individualized experience, with recommendations tailored almost exclusively to a single user’s viewing history.

We propose a standalone website that brings collaborative discovery to YouTube. Users will be able to join chatrooms with friends or family, where the system generates a blended stream of recommended videos based on the combined likes, dislikes, and viewing patterns of everyone in the room. This approach helps users break out of stale recommendation loops and discover new content influenced by the people closest to them—without relying on guesswork or manually sharing links.

# Technical Description

The project will be implemented as a standalone web application supporting multiple users and shared sessions. A centralized backend will manage user authentication, chatroom state, and the blended recommendation logic, while persisting user preferences and interaction data across sessions. The application will be accessible on both desktop and mobile platforms, with an emphasis on clarity, responsiveness, and maintainable system architecture.

The backend will be written primarily in C++, using a lightweight web framework to expose REST-style endpoints for login, session handling, chatroom operations, and recommendation retrieval. Due to the nature of YouTube’s API and OAuth 2.0, some components—specifically those handling authorization and API requests—will be implemented in Python. These Python utilities will retrieve YouTube metadata and user preference data, which will then be processed by the C++ backend. As a fallback option, users may also import their own data through Google Takeout exports.

To maintain portability and minimize external dependencies, the backend will rely on core C++ libraries for HTTP routing, JSON serialization, and CSV/HTML parsing. Metadata obtained from YouTube or user-provided files will feed into our recommendation engine, which blends user preferences by analyzing watch history, playlist data, and content features to generate a shared video feed for each chatroom.

User accounts, chatroom state, and preference data will be stored in an embedded SQLite database to ensure lightweight persistence and ease of deployment across Unix-like environments. A small portion of the codebase will consist of HTML, CSS, and minimal JavaScript to provide a clean and functional web interface for interacting with the platform.
