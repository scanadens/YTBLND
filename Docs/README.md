# README

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

***Authored***: Shamar Pennant

</aside>

# YTBLND

A C++ desktop application that blends YouTube Watch Later data from multiple users to generate a shared recommendation feed. Users can create blends, browse a combined video feed, and chat within blend‑specific rooms.

## Overview

YTBLND is built with a modular architecture separating UI, business logic, data parsing, algorithms, and persistence. The application uses Google Takeout CSV exports to ingest YouTube data and stores all user and blend information in a local SQLite database.

## Features

- User registration and login
- Upload Watch Later CSV (Google Takeout)
- Create blends with 2–8 participants
- Randomized blended video feed (3×2 grid)
- Paging through feed results
- Blend‑specific chatroom
- Persistent storage using SQLite

## Dependencies

Required:

- CMake 3.16+
- C++20 compiler (GCC/Clang)
- wxWidgets (core, base, net)
- SQLite3 and SQLiteCpp
- GoogleTest (for tests)

Optional:

- Gumbo HTML parser (enables HTML history parsing)

If Gumbo is not installed, HTML parsing is automatically disabled.

## Building

```bash
./build_src.sh
```

This produces:

- `ytblnd` — main application
- `ytblnd_tests` — unit tests

## Running

To run the application after building the project with the above command, use the following command below in the project root:

```bash
./run_ytblnd_proto.sh
```

## Running Tests

There is compiled gtests found within the directory `YTBLND_src/tests/`. Which after building the project can be run with the following command from the project root:

```bash
./run_test.sh
```

For additional testing purposes while running the application, there are provided data pieces (playlist files as `.csv`) within `YTBLND_src/tests/fixtures/`. These can be used for when creating user profiles if one doesn’t wan’t to provide their own.

## Project Structure

```
YTBLND_src/
├── AlgorithmLayer/        # Blend algorithms
├── AppInfrastructure/     # CSV/HTML parsing, data extraction
├── AppLayer/              # Controller, state, event routing
├── ModelLayer/            # User, Video, Blend, ChatRoom
├── ServiceLayer/          # SQLite persistence
├── UILayer/               # wxWidgets UI panels
└── tests/                 # GoogleTest suites
```

## Data Requirements

The application expects a csv file upon creating an account. Usually comprised of playlist data which can be found within the following directory structure after downloading YouTube data from [Google Takeout](https://takeout.google.com):

```bash
YouTube and YouTube Music/playlists/
```

# Known Bugs & Issues:

So far with our development, there are a couple of known issues which comprises of the following:

- Chatroom does not display messages after being sent.
- Chatroom messages are not viewable by others that are a part of the blend
- Can generate blend by yourself (creating a blend with a user that doesn’t exist yet)
- App crashes after multiple refreshes of the blend
- There’s no means of editing user data after account creation
- No over the network syncing; meaning saved data is only local to a machine