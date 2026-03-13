# Prototype Details:

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

***Authored***: Shamar Pennant

</aside>

As of March 12, 2026 

Many revisions has been made to our documentation as well as changes to code implementation. Some of which we realized along the way was needed for our algorithms to be more cohesive and exhibit standard coding practices learned from class and design patterns found online.

Additionally, changes were made and features were left out to produce a minimal working prototype. Which include the following changes listed below:

## **Architecture & Platform**

- Shifted from a planned **web application** to a fully **desktop application** using wxWidgets.
- Removed the planned **C++ web server**, REST endpoints, and browser‑based UI.
- Introduced a multi‑panel desktop UI with Login, Data Upload, Blend Creation, Home Feed, User Page, and Chat.

## **Data Ingestion**

- Removed **OAuth 2.0** and all YouTube API integration.
- Removed the Python component originally intended for API calls.
- Simplified ingestion to **only Google Takeout Watch Later CSV**.
- Dropped support for subscriptions, watch history HTML, and search history HTML.

## **Recommendation Engine**

- Replaced the planned multi‑stage ML‑style blending engine with a **RandomBlendAlgorithm**.
- Removed tag extraction, feature vectors, similarity metrics, and explainability cues.
- Removed diversification, weighting, and multi‑user profile modeling.

## **Persistence Layer**

- Fully implemented a **SQLite‑based persistence system** with tables for users, watch later data, blends, and participants.
- Added automatic blend regeneration on login.
- Implemented clean re‑upload behavior (delete + reinsert rows).
- Added deterministic state management for blends and chatrooms.

## **Application Flow & State Management**

- Designed and implemented a complete **page flow** with validation rules for every panel.
- Added `AppState` to track current user, active blend, chatroom, and session data.
- Added `AppController` and `EventRouter` to manage all app events and transitions.
- Implemented detailed validation for login, registration, blend creation, and data upload.

## **Chatroom Feature**

- Moved chatroom from “wish list” to implemented prototype feature.
- Added chatroom panel, state tracking, and message dispatch events (logic stubbed).

## **Codebase Structure**

- Expanded the original simple class layout into a **multi‑layer architecture**:
    - AlgorithmLayer
    - AppInfrastructure
    - AppLayer
    - ModelLayer
    - ServiceLayer
    - UILayer
- Added extensive GoogleTest coverage for parsing, persistence, and algorithm behavior.

## **UI & Interaction**

- Implemented a functional 3×2 video feed grid with paging.
- Added placeholder states for empty blends.
- Added dialogs for errors, warnings, and missing data.
- Added navigation logic for all panels, including back‑button behavior.