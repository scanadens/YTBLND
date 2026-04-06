# Project Structure and UML

This document reflects the current repository layout and source modules in
`YTBLND_src/` as of April 5, 2026.

## High-Level Structure

```text
group45/
├── CMakeLists.txt                 # Root CMake (adds YTBLND_src/, doc target)
├── Docs/                          # Product/design/project docs
├── YTBLND_src/                    # Main source tree (C++ app + tests + Go backend)
│   ├── AlgorithmLayer/            # Blend selection algorithms
│   ├── AppInfrastructure/         # Input parsing/import and file utilities
│   ├── AppLayer/                  # Controller/state/event orchestration
│   ├── ModelLayer/                # Core domain entities
│   ├── ServiceLayer/              # HTTP/WebSocket/metadata/persistence adapters
│   ├── UILayer/                   # wxWidgets UI components
│   ├── tests/                     # GoogleTest suites and fixtures
│   ├── ytblnd-backend/            # Go backend server and route/database layers
│   ├── main.cpp                   # wxWidgets application entry point
│   ├── ServerConfig.hpp           # backend URL/config constants
│   └── CMakeLists.txt             # Main build logic and feature toggles
├── build_src.sh                   # Default build wrapper
├── build_dev_src.sh               # Dev/test build wrapper
├── run_tests.sh                   # Test runner wrapper
└── generate_docs.sh               # Doxygen/PDF documentation wrapper
```

## Source Modules by Layer

### AlgorithmLayer

Implements the blending strategy contract used by app/business logic.

- `IBlendAlgorithm.hpp`
- `RandomBlendAlgorithm.hpp`, `RandomBlendAlgorithm.cpp`
- `WeightedBlendAlgorithm.hpp`

### AppInfrastructure

Parsing, source loading, extraction, and import helpers for YouTube data files.

- `CsvParser.hpp`, `CsvParser.cpp`
- `CsvSource.hpp`, `CsvSource.cpp`
- `DataExtractor.hpp`, `DataExtractor.cpp`
- `DataSaver.hpp`
- `FileBin.hpp`
- `FileFormatter.hpp`
- `File_ID.hpp`
- `FileSource.hpp`
- `HtmlParser.cpp` (conditionally compiled; excluded when Gumbo is unavailable)
- `HtmlSource.cpp` (conditionally compiled; excluded when Gumbo is unavailable)
- `Parser.hpp`
- `WatchHistoryParser.hpp`, `WatchHistoryParser.cpp`
- `WatchLaterParser.hpp`, `WatchLaterParser.cpp`
- `YouTubeDataImporter.hpp`, `YouTubeDataImporter.cpp`

### AppLayer

Application orchestration and event routing between UI and services.

- `AppController.hpp`, `AppController.cpp`
- `AppState.hpp`, `AppState.cpp`
- `EventRouter.hpp`, `EventRouter.cpp`

### ModelLayer

Domain models for users, videos, blends, and chat entities.

- `Blend.hpp`, `Blend.cpp`
- `Channel.hpp`, `Channel.cpp`
- `ChatRoom.hpp`, `ChatRoom.cpp`
- `Friend.hpp`, `Friend.cpp`
- `JsonUtils.hpp`
- `Message.hpp`, `Message.cpp`
- `User.hpp`, `User.cpp`
- `Video.hpp`, `Video.cpp`
- `VideoEntry.hpp`, `VideoEntry.cpp`
- `YouTubeData.hpp`, `YouTubeData.cpp`

### ServiceLayer

Service adapters for persistence, backend communication, and metadata requests.

- `DataManager.hpp` (persistence interface)
- `SqliteDataManager.hpp`, `SqliteDataManager.cpp` (optional build path)
- `HttpClient.hpp`, `HttpClient.cpp`
- `RequestJsonBuilder.hpp`, `RequestJsonBuilder.cpp`
- `ChatWebSocket.hpp`, `ChatWebSocket.cpp`
- `YouTubeMetadataFetcher.hpp`, `YouTubeMetadataFetcher.cpp`

### UILayer

wxWidgets panels, widgets, and style/config utilities.

- `MainFrame.hpp`, `MainFrame.cpp`
- `LoginPanel.hpp`, `LoginPanel.cpp`
- `DataInstructionsPanel.hpp`, `DataInstructionsPanel.cpp`
- `BlendCreationPanel.hpp`, `BlendCreationPanel.cpp`
- `BlendFeedPanel.hpp`, `BlendFeedPanel.cpp`
- `BlendChatPanel.hpp`, `BlendChatPanel.cpp`
- `ActiveBlendsPanel.hpp`, `ActiveBlendsPanel.cpp`
- `UserPanel.hpp`, `UserPanel.cpp`
- `SettingsPanel.hpp`, `SettingsPanel.cpp`
- `TopBar.hpp`, `TopBar.cpp`
- `VideoCard.hpp`, `VideoCard.cpp`
- `ConfirmationDialog.hpp`, `ConfirmationDialog.cpp`
- `OperationProgressDialog.hpp`, `OperationProgressDialog.cpp`
- `ButtonsConfig.hpp`, `ButtonsConfig.cpp`
- `UIColors.hpp`, `UIColors.cpp`
- `ResourcePathUtils.hpp`, `ResourcePathUtils.cpp`
- `IRefreshablePanel.hpp`
- `UIPages.hpp`

### tests

GoogleTest suites for model logic, app infrastructure, service wrappers, auth,
UI behavior (optionally), and live backend connectivity.

- `Test_AppInfr.cpp`
- `Test_Auth.cpp` (SQLite-dependent)
- `Test_BlendCreationPanel.cpp` (UI test)
- `Test_BlendManagement.cpp`
- `Test_ChatPersistence.cpp`
- `Test_LiveBackendConnectivity.cpp`
- `Test_Model.cpp`
- `Test_ModelToString.cpp`
- `Test_ServiceLayerWrappers.cpp`
- `Test_ThemeSwitching.cpp` (UI test)
- `Test_WatchLaterBlend.cpp`
- `fixtures/` (CSV/HTML test inputs)

### ytblnd-backend (Go)

Server-side auth/blends/chat routes and SQLite-backed data layer. Used to reflect how the server works.

- `main.go`
- `route_layer/` (`auth.go`, `blends.go`, `chat.go`, `logging.go`, tests)
- `database_layer/` (`DataManager.go`, `SqliteDatabaseManager.go`, `models.go`, tests)
- `backup_manager.go`, `db_migration.go` and related tests
- `go.mod`, `go.sum`, `Makefile`

## Build and Target Notes (from CMake)

- Root `CMakeLists.txt` sets C++20, enables `compile_commands.json`, and adds
    `YTBLND_src` as the main project.
- Main executable target: `ytblnd` (only built when wxWidgets is found).
- Test executable target: `ytblnd_tests` (built when `YTBLND_BUILD_TESTS=ON`).
- `SqliteDataManager.cpp` and SQLite-dependent tests are excluded unless
    `YTBLND_ENABLE_SQLITE=ON` and SQLite is found.
- `HtmlParser.cpp` and `HtmlSource.cpp` are excluded when Gumbo is not found.
- External dependencies fetched automatically with FetchContent:
    IXWebSocket and nlohmann/json.

## UML Class Diagram

Existing diagram asset:

![YT-BLND UML](Resources/YT-BLND-uml-diag-Page-1.drawio.svg)

Note: The current UML image may lag behind the live code structure above and
should be refreshed if strict class-level parity is required.