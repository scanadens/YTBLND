# Other Notes

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" /> ***Contributors:*** Jasmine Kumar, Shamar Pennant, Yousef Selim

</aside>

# YouTube data authorization:

- Oauth 2.0
- scopes: (for more info click [here](https://developers.google.com/identity/protocols/oauth2/scopes#youtube))

| [`https://www.googleapis.com/auth/youtube.readonly`](https://www.googleapis.com/auth/youtube.readonly) | live login and access users’ private and public playlists (including liked videos) and subscription list |
| --- | --- |
| [`https://www.googleapis.com/auth/youtube.force-ssl`](https://www.googleapis.com/auth/youtube.force-ssl) | access users’ watch history |

### Alternatively:

- users export their own data using [google takeout](https://takeout.google.com/?pli=1)
    - minimizes interfacing with YouTube’s API and mitigates need for using Oauth 2.0
    - simpler for coding, get the .csv file directly
    - less convenient for the user

### File Formats

- history - watch and search
    - html docs
- playlists
    - csv files
- subscriptions
    - csv file

# Environment:

- Linux

# Web Front End:

- The front end will comprise for a small percentage of our code base (~10%) and will use the following:
    - HTML - website formatting and file upload forms
    - Minimal JS - dynamic behavior and interactivity
    - CSS - add a stylized retro feel to the website

# Backend Libraries & Logistics:

To keep the system portable and buildable in a Linux environment, the recommendation engine and supporting backend will rely on lightweight, well-supported C++ libraries:

- **HTTP server/API layer**: a small C++ web framework (e.g., Crow or Pistache) to expose endpoints for login/session handling, chatroom actions, and recommendation requests.
- **JSON serialization:** a JSON library (e.g., nlohmann/json) for request/response payloads and internal data interchange.

## Blending Algorithm Engine:

- **Data parsing**:
    - CSV parsing for playlists/subscriptions exports (either a minimal custom parser or a small CSV utility).
    - HTML parsing for Google Takeout history files (prefer simple pattern extraction where feasible; otherwise use a lightweight HTML parser such as [Gumbo](https://codeberg.org/gumbo-parser/gumbo-parser)).
- **Libraries to consider:**
    - Eigen3 - linear algebra
    - Boost - utilities
        - Boost.Math - statistics
        - Boost.Random - controlled randomness
        - Boost.Graph - network analysis
    - nlohmann/json - JSON library
    - ONNX Runtime - if we want to use pre-trained models
    - Gumbo - HTML parser

### Algorithm Pipeline

Operationally, the engine will be split into five stages:

1. **Data Preprocessing:** 
    - filter videos - remove shorts & duplicates, filter by watch duration)
    - extract features - categories from titles, channels, etc.
    - normalization - scale each user’s watch count and weigh videos based on recent activity
2. **Profile Building**:
    - create a set of dimensions for each user (can scale and add more as we progress for more fine-tuned recommendations)
        - result is a multi-dimensional vectors that lets us compare users and recommend videos based on their preferences
3. **Similarity Calculation:** 
    - score candidate videos using the blended profile
    - calculate user-user similarity including (but not limited to):
        - category overlap - Jaccard
        - watch history similarity - Cosine
        - channel subscription overlap
        - combined similarity = weighted avg of above
4. **Recommendation Generation**
    - select a blending strategy and apply content controls (blocked tags), then return a list with explainability cues (top tags, contributing members).
5. **Post-Processing**
    - diversification - make sure all videos are not from same few channels and mix the categories
    - generate explanations - like mentioned above, “Recommended because both of you watch X” or “User A liked this”, “User B watches this channel”
    - ordering - group by a theme or try to create a flow between the videos

## State Management

The backend will maintain consistent state for users and chatrooms, including membership, roles/permissions, and the currently generated blended feed. To reduce complexity, the initial approach will treat the backend as the single source of truth, with clients acting as thin views over server state.

Key logistical considerations include:

- Session management: maintain authenticated sessions securely and expire sessions appropriately.
- Room state consistency: ensure joins/leaves/bans update room membership and blended profiles deterministically.
- Persistence strategy: write updates atomically (e.g., write-then-rename) to avoid corrupting stored state.
- Rate limits/failure handling: gracefully handle missing/partial YouTube data and API quota limitations if OAuth is used.

### Database:

- embedded SQLite database to save user’s data across sessions - create a database class
- **Dependencies:**
    - SQLiteCPP library
    - SQLite3

## Testing and Validation

Core system components such as the blending algorithm, similarity calculations, and permission logic will be validated through unit and integration testing. Testing will focus on correctness, consistency of recommendations, and robustness under edge cases such as users joining or leaving active blends.

We will be using the [gtest](https://google.github.io/googletest/) library to conduct our C++ unit testing.