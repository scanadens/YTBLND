<aside>
<img src="https://www.notion.so/icons/brain_orange.svg" alt="https://www.notion.so/icons/brain_orange.svg" width="40px" />

>**Contributors**: Shamar Pennant, Jasmine Kumar, Xavier Lusetti, Yousef Selim
>

</aside>

### Front End

- interface may not be feasible in majority c++ code
- phone UI may be clunky as a web app

### Back End

- Implementation of a generative algorithm based on tags is foreign to us
- complexity involved with blending algorithm - machine learning model
    - may involve need for databases - data persistence
    - unknown how YouTube videos available for recommendation will be fed into the algorithm
- recommendations may not feel intuitive or untrustworthy to users if the blending logic is opaque or difficult to interpret
- maintaining consistent chatroom state, user permissions, and blended recommendations across multiple active users may introduce back end synchronization and persistence challenges

### Authorization

- interfacing with YouTube’s API — written in python may prove difficult as both have different architectural philosophies

### General

- scope creep can make our features too overly complicated

## Strategies for Mitigating

- need to research the details of the YouTube API and its requirements
- if permission granting for accounts is too robust, we can ask users to select their interests and then generate content from that
- researching the minimal components of each feature to minimize scope creep
    - one strategy to avoid scope creep with user authentication would be to create a blending algorithm using user data from google takeout and if time allows use the oauth authorization (can get the same data using both)
    - incorporating recommendation transparency and user feedback mechanisms to help users understand and influence how blended content is generated
    - designing a centralized back end state model with clear ownership of chatroom data and atomic persistence operations to ensure consistency across user sessions
- videos that populate the recommendation feed could be a subset of videos seen/liked by participating users in order to mitigate the need for external videos that populate the feed
    - if time permits we can fetch fresh videos through YouTube API but this adds additional complexity
- For data persistence an SQLite database can be setup in C++
    - less of an overkill than a MySQL server and lets us focus on functional code rather than database administration while allowing for data persistence across different sessions
- develop a separate HTML layout for a phone in order to make it more palatable on a smaller screen
- Similarity computations - use basic vector math utilities implemented in-house (cosine similarity/Jaccard overlap) to avoid heavy ML dependencies.
