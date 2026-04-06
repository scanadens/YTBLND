# README

<aside>
<img src="https://www.notion.so/icons/pencil_orange.svg" alt="https://www.notion.so/icons/pencil_orange.svg" width="40px" />

***Authored***: Shamar Pennant 

</aside>

[README](https://www.notion.so/README-33a12276ddb480fa97fdf3d748d4d390?pvs=21)

# YTBLND

A C++ desktop application that blends YouTube playlist or watch history data from multiple users to generate a shared recommendation feed. Users can create blends, browse a combined video feed, and chat within blend‑specific rooms.

## Overview

YTBLND is built with a modular architecture separating UI, business logic, data parsing, algorithms, and persistence. The application uses Google Takeout CSV and HTML exports to ingest YouTube data and relies on a backend service for live data persistence. SQLite is still supported for local/dev and selected test paths. Video metadata (titles, channel names, thumbnails) is enriched via the YouTube Data API v3.

## Features

- User registration and login
- Upload YouTube playlist CSV or watch-history (Google Takeout)
- Automatic metadata enrichment via YouTube Data API v3 (titles, channel names, thumbnails, channel logos)
- Create blends with 2–8 participants
- Randomized blended video feed (3×2 grid)
- Paging through feed results through a refresh button
- Blend‑specific chatroom
- Server-backed persistence (SQLite used in backend/local-dev paths)

## Dependencies

The default app build only needs the runtime/build dependencies below. Test-only and feature-gated dependencies are listed separately.

### Required

| Dependency | Arch Linux | Ubuntu / Debian | Fedora / RHEL / CentOS | Windows (MSYS2 MinGW‑w64) | macOS (Homebrew) |
| --- | --- | --- | --- | --- | --- |
| CMake 3.16+ | `sudo pacman -S cmake` | `sudo apt install cmake` | `sudo dnf install cmake` | `pacman -S mingw-w64-x86_64-cmake` | `brew install cmake` |
| C++20 compiler (GCC or Clang) | `sudo pacman -S gcc` | `sudo apt install g++` | `sudo dnf install gcc gcc-c++` | `pacman -S mingw-w64-x86_64-gcc` | `brew install llvm` *(or use system Clang)* |
| wxWidgets (core, base, net) | `sudo pacman -S wxwidgets-gtk3` | `sudo apt install libwxgtk3.2-dev` | `sudo dnf install wxGTK3-devel` | `pacman -S mingw-w64-x86_64-wxWidgets3.2` | `brew install wxwidgets` |
| libcurl | `sudo pacman -S curl` | `sudo apt install libcurl4-openssl-dev` | `sudo dnf install libcurl-devel` | `pacman -S mingw-w64-x86_64-curl` | `brew install curl` |

### Optional

| Dependency | Purpose | Arch Linux | Ubuntu / Debian | Fedora / RHEL / CentOS | Windows (MSYS2 MinGW‑w64) | macOS (Homebrew) |
| --- | --- | --- | --- | --- | --- | --- |
| GoogleTest | Builds `ytblnd_tests` and enables `run_tests.sh` / `test_live_backend_connectivity.sh` | `sudo pacman -S gtest` | `sudo apt install libgtest-dev` | `sudo dnf install gtest-devel` | `pacman -S mingw-w64-x86_64-gtest` | `brew install googletest` |
| SQLite3 | Enables local SQLite persistence and SQLite-dependent tests | `sudo pacman -S sqlite` | `sudo apt install libsqlite3-dev` | `sudo dnf install sqlite-devel` | `pacman -S mingw-w64-x86_64-sqlite3` | `brew install sqlite` |
| Gumbo HTML parser | Enables watch-history HTML parsing | `sudo pacman -S gumbo-parser` | `sudo apt install libgumbo-dev` | `sudo dnf install gumbo-parser gumbo-parser-devel` | *Not packaged — build from source* | `brew install gumbo-parser` |
| pkg-config | Lets CMake auto-detect Gumbo on Linux | `sudo pacman -S pkgconf` | `sudo apt install pkg-config` | `sudo dnf install pkgconf-pkg-config` | `pacman -S pkgconf` | `brew install pkg-config` |
| Doxygen | Generates API documentation | `sudo pacman -S doxygen` | `sudo apt install doxygen` | `sudo dnf install doxygen` | `pacman -S doxygen` | `brew install doxygen` |

If Gumbo or `pkg-config` is not installed, HTML parsing is automatically disabled and only CSV Watch Later exports are supported.

### Fetched automatically by CMake

These libraries are downloaded by CMake’s `FetchContent` during the first configure step and do not need to be installed manually:

- **nlohmann/json v3.11.3** — JSON parsing for YouTube API responses
- **IXWebSocket v11.4.5** — WebSocket client for the blend chat feature

An internet connection is required the first time you run `cmake -S . -B build`. After that the libraries are cached in `build/_deps/`.

## Building

From the project root (`group45/`):

```bash
./build_src.sh
```

This configures the default app build in `build/` with tests disabled and SQLite turned off, so end-user installs do not require test-only dependencies. Takes no arguments; Gumbo and Doxygen presence is auto-detected and reported.

Or manually:

```bash
cmake -S . -B build -DYTBLND_BUILD_TESTS=OFF -DYTBLND_ENABLE_SQLITE=OFF
cmake --build build --parallel
```

For the dev/test build with SQLite and test targets enabled:

```bash
./build_dev_src.sh

# Or manually
cmake -S . -B build-dev -DYTBLND_BUILD_TESTS=ON -DYTBLND_ENABLE_SQLITE=ON
cmake --build build-dev --parallel
```

`build_dev_src.sh` takes no positional arguments. If Doxygen is installed, the `doc` target is built automatically after the main build. To force docs generation and fail if Doxygen is missing, set `YTBLND_DEV_BUILD_DOCS=on` before running:

```bash
YTBLND_DEV_BUILD_DOCS=on ./build_dev_src.sh
```

Valid values for `YTBLND_DEV_BUILD_DOCS`: `auto` (default — build docs if Doxygen is present), `on` / `true` / `1` (require docs), anything else skips docs.

If you have a stale build directory (e.g. after adding new CMake dependencies), wipe it first:

```bash
rm -rf build && cmake -S . -B build && cmake --build build --parallel
```

This produces:

- `build/YTBLND_src/ytblnd` — main application
- `build-dev/YTBLND_src/ytblnd_tests` — unit/integration tests from the dev build

## Running

To run the application after building:

```bash
./run_ytblnd_proto.sh
```

## Linux Launcher

A Linux desktop entry is generated by CMake at `build/linux/ytblnd.desktop` when the `ytblnd` target is configured.

This is separate from a CMake build target. A shell script like `build_src.sh` is only a wrapper around CMake commands; the actual CMake targets are things like `ytblnd`, `ytblnd_tests`, and `install`.

To install it for your current user:

```bash
cmake -S . -B build
mkdir -p ~/.local/share/applications
cp build/linux/ytblnd.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications 2>/dev/null || true
```

The generated launcher uses absolute paths from the current configure step. If the repository or build directory moves, rerun `cmake -S . -B build` to regenerate it.

For convenience, CMake also exposes a `ytblnd_linux_desktop_entry` target that depends on the app target and keeps the launcher generation grouped with the build.

### Linux Install

For a quick local install on Linux, use [install_local.sh](install_local.sh):

```bash
./install_local.sh [prefix]
```

That script configures CMake with `CMAKE_INSTALL_PREFIX=$HOME/.local`, builds the app, runs `cmake --install build`, and refreshes the local desktop metadata. After that, the app binary is installed under `~/.local/bin`, the icon under `~/.local/share/icons/...`, and the desktop entry under `~/.local/share/applications/ytblnd.desktop`.

If you want the separate dev install with SQLite enabled, use:

```bash
./install_local_dev.sh [prefix]
```

Both install scripts accept an optional first argument to override the install prefix, e.g. `./install_local_dev.sh /opt/ytblnd`. `install_local_dev.sh` also respects `YTBLND_DEV_BUILD_DOCS` (same values as `build_dev_src.sh`) to control whether the `doc` target is built during install.

If you prefer to run the install steps manually:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
```

Cross-platform note: the `.desktop` file is Linux-only. The runtime window icon is set in the wxWidgets app itself, so the app window can use the same icon on Linux, Windows, and macOS, but each OS needs its own launcher/package metadata:

- Linux uses a `.desktop` file and PNG icon paths.
- Windows uses executable resources, typically via an `.ico` file.
- macOS uses app bundle metadata with an `.icns` file.

## Running Tests

Unit tests are in `YTBLND_src/tests/`. After building, run them with:

```bash
./build_dev_src.sh
./run_tests.sh [gtest flags]
```

`run_tests.sh` forwards any extra arguments directly to the gtest binary, so you can pass standard gtest flags:

```bash
./run_tests.sh --gtest_filter='CsvParser*'
./run_tests.sh --gtest_repeat=3 --gtest_shuffle
```

Or run the binary directly:

```bash
./build-dev/YTBLND_src/ytblnd_tests
```

To run only the live backend connectivity tests (backend server as of April 5, 2026 is currently running on a [vultr](https://www.vultr.com) cloud instance in Chicago):

```bash
./test_live_backend_connectivity.sh
```

`test_live_backend_connectivity.sh` always applies `--gtest_filter='LiveBackendConnectivityTest.*'` and sets `YTBLND_RUN_LIVE_BACKEND_TESTS=1` for that test process. Any extra arguments you pass are forwarded to the gtest binary after the filter, so you can still use flags like `--gtest_repeat` or `--gtest_shuffle`:

```bash
./test_live_backend_connectivity.sh --gtest_repeat=2
```

If the full dev test run passes but the isolated live-backend script fails, investigate the script or environment assumptions first rather than assuming the backend itself is broken.

For manual testing, sample Watch Later CSV files are provided in `YTBLND_src/tests/fixtures/`.

## Generating Documentation

For HTML docs only (no LaTeX/PDF required):

```bash
cmake --build build --target doc
```

Output is written to `build/doxygen_output/html/index.html`.

To generate both HTML and PDF documentation:

```bash
./generate_docs.sh
```

`generate_docs.sh` takes no arguments. It checks for `doxygen` and `pdflatex` before running; if either is missing it prints install instructions and exits. On success the PDF is copied to `Docs/refman.pdf`.

Requires `pdflatex` from a LaTeX distribution:

| Distro | Install |
| --- | --- |
| Ubuntu / Debian | `sudo apt install texlive-latex-base texlive-latex-extra texlive-fonts-recommended` |
| Fedora / RHEL | `sudo dnf install texlive-collection-latexextra texlive-collection-fontsrecommended` |
| Arch Linux | `sudo pacman -S texlive-latex texlive-latex-extra` |

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

The application expects a YouTube playlist CSV or watch-history HTML export from Google Takeout. After downloading your YouTube data, the file is found at:

```bash
Takeout/YouTube and YouTube Music/playlists/
#and or
Takeout/YouTube and YouTube Music/history/watch-history.html
```

Visit [Google Takeout](https://takeout.google.com/) to download your YouTube data.

## Known Bugs & Issues

- Theme selection is limited to the current build session (is not backed by the server and refreshes on building src)
- Certain account logins redirect to other accounts being logged in instead
- Blend does not filter out ads from `watch-history.html`
- App installation is currently limited to Linux
- There is no means of the user being able to save a copy of their YouTube data through the app
- Unable to add other users within the app as a standalone friend without creating a blend with them
- No meta data tags or categorization for the blend feed (as a result can't block videos through tag selection)
- There are no super privileges within chatrooms (can't kick people out) 
- Full legitimate (or at least streamlined) installation is limited to Linux 