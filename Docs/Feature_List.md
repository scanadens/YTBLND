# Feature List

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

> **Contributors**: Shamar Pennant, Xavier Lusetti, Jasmine Kumar, Yousef Selim, Sophie Smith Stewart
**Author**: Shamar Pennant, Jasmine Kumar
> 
</aside>

# major ideas:

- have a cohesive appearance and design
- aesthetic is akin to Y2K vibes, while still maintaining functionality and intentional design
- is not clunky, and is viewable in both desktop and phone platforms

## systems to implement

### required

1. user experience and settings
    - have access to peoples YouTube data through CSV file user uploads
    - be able to blend the participating users videos 
2. recommendation engine
    - suggests videos based on the poeple in the blend's CSV file uploads
3. back end infrastructure
    1. web interface (have it be accessible via website)
4. personal data management
    - authentication / users export own data
    - account deletion
    - account creation
6. Data Persistence
    - Blends the user is part of are saved 
    - Users CSV saved accross sessions
7. System Architecture
    - separate concerns such as:
        - user management
        - recommendation logic
        - chatroom state
        - data persistence
    - this will allow for components to be developed, tested, and modified individually to reduce coupling

# optional for enhanced usability

- be able to blend YouTube shorts
- oauth authentication
- scale for more users

# wish list

- Allow users to give explicit feedback to the recommendation system (e.g., “show more like this” / “show less like this”) to refine future 
- save chatroom history
- be able to take a given list of YouTube meta tags and produce suggestions
- allows adjustments based on user feedback and content control rather than a fully opaque machine learning model
- extract tags from liked videos, watch/search history
- be able to seamlessly integrate into the browser as a plugin
- Have pre-selected themes for users to choose from per each users individual view
- chatroom
    - have each blend associated with a chat room
        - blends full length YouTube videos
    - creator of chatroom has highest privileges and can give invites/ban people from the room. or can grant other the same privileges
    - can hold up to 2-8 people
    - show who the video is recommended/generated from
    - how similar your feed is to another person in the group chat as a percentage
        - ability to view participating members’ details on their feed similarities and likes/dislikes (see common interests)
        - can be seen in a side peek view