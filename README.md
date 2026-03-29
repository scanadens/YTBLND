# README

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

***Authored***: Shamar Pennant

</aside>

# YTBLND

A C++ desktop application that blends YouTube Watch Later data from multiple users to generate a shared recommendation feed. Users can create blends, browse a combined video feed, and chat within blend‑specific rooms.

## Overview

YTBLND is built with a modular architecture separating UI, business logic, data parsing, algorithms, and persistence. The application uses Google Takeout CSV exports to ingest YouTube data and stores all user and blend information in a local SQLite database. Video metadata (titles, channel names, thumbnails) is enriched via the YouTube Data API v3.

## Features

- User registration and login
- Upload Watch Later CSV (Google Takeout)
- Automatic metadata enrichment via YouTube Data API v3 (titles, channel names, thumbnails, channel logos)
- Create blends with 2–8 participants
- Randomized blended video feed (3×2 grid)
- Paging through feed results
- Blend‑specific chatroom
- Persistent storage using SQLite

## Dependencies

The following must be installed on your system before building.

### Required

| Dependency | Arch Linux | Ubuntu / Debian |
|---|---|---|
| CMake 3.16+ | `sudo pacman -S cmake` | `sudo apt install cmake` |
| C++20 compiler (GCC or Clang) | `sudo pacman -S gcc` | `sudo apt install g++` |
| wxWidgets (core, base, net) | `sudo pacman -S wxwidgets-gtk3` | `sudo apt install libwxgtk3.2-dev` |
| libcurl | `sudo pacman -S curl` | `sudo apt install libcurl4-openssl-dev` |
| SQLite3 | `sudo pacman -S sqlite` | `sudo apt install libsqlite3-dev` |
| GoogleTest | `sudo pacman -S gtest` | `sudo apt install libgtest-dev` |
| pkg-config | `sudo pacman -S pkgconf` | `sudo apt install pkg-config` |

### Optional

| Dependency | Purpose | Arch Linux | Ubuntu / Debian |
|---|---|---|---|
| Gumbo HTML parser | Enables watch-history HTML parsing | `sudo pacman -S gumbo-parser` | `sudo apt install libgumbo-dev` |
| Doxygen | Generates API documentation | `sudo pacman -S doxygen` | `sudo apt install doxygen` |

If Gumbo is not installed, HTML parsing is automatically disabled and only CSV Watch Later exports are supported.

### Fetched automatically by CMake

These libraries are downloaded by CMake's `FetchContent` during the first configure step and do not need to be installed manually:

- **nlohmann/json v3.11.3** — JSON parsing for YouTube API responses
- **IXWebSocket v11.4.5** — WebSocket client for the blend chat feature

An internet connection is required the first time you run `cmake -S . -B build`. After that the libraries are cached in `build/_deps/`.

## Building

From the project root (`group45/`):

```bash
./build_src.sh
```

Or manually:

```bash
cmake -S . -B build
cmake --build build --parallel
```

If you have a stale build directory (e.g. after adding new CMake dependencies), wipe it first:

```bash
rm -rf build && cmake -S . -B build && cmake --build build --parallel
```

This produces:

- `build/YTBLND_src/ytblnd` — main application
- `build/YTBLND_src/ytblnd_tests` — unit tests

## Running

To run the application after building:

```bash
./run_ytblnd_proto.sh
```

## Running Tests

Unit tests are in `YTBLND_src/tests/`. After building, run them with:

```bash
./run_tests.sh
```

Or directly:

```bash
./build/YTBLND_src/ytblnd_tests
```

For manual testing, sample Watch Later CSV files are provided in `YTBLND_src/tests/fixtures/`.

## Generating Documentation

```bash
cmake --build build --target doc
```

Output is written to `Docs/html/index.html`.

## Project Structure

```
YTBLND_src/
├── AlgorithmLayer/        # Blend generation algorithms (IBlendAlgorithm interface)
├── AppInfrastructure/     # CSV/HTML parsing, data extraction utilities
├── AppLayer/              # AppController, AppState, EventRouter
├── ModelLayer/            # Data models: User, Video, Blend, ChatRoom, etc.
├── ServiceLayer/          # SQLite persistence, HTTP client, YouTube API fetcher, WebSocket
├── UILayer/               # wxWidgets UI panels and widgets
├── tests/                 # GoogleTest suites
└── ytblnd-backend/        # Go backend server (chat persistence, auth routes)
```

## Data Requirements

The application expects a Watch Later CSV export from Google Takeout. After downloading your YouTube data, the file is found at:

```
YouTube and YouTube Music/playlists/Watch later-videos.csv
```

Visit [Google Takeout](https://takeout.google.com) to download your YouTube data.

## Known Bugs & Issues

- Chatroom does not display messages after being sent.
- Chatroom messages are not viewable by others in the blend.
- A blend can be generated with a single user or a non-existent user.
- App crashes after multiple refreshes of the blend.
- No means of editing user data after account creation.
