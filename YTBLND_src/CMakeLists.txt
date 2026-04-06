# ── Source globs ─────────────────────────────────────────────────────────────

include(${CMAKE_SOURCE_DIR}/cmake/GenerateLinuxDesktopLauncher.cmake)

option(YTBLND_BUILD_TESTS "Build unit tests" OFF)
option(YTBLND_BUILD_UI_TESTS "Build wxWidgets/UI tests" OFF)
option(YTBLND_REQUIRE_WX "Fail configure if wxWidgets is missing" OFF)
option(YTBLND_ENABLE_SQLITE "Enable SQLite-backed local persistence (used by local tests/dev)" OFF)
option(YTBLND_FETCHCONTENT_UPDATES_DISCONNECTED "Skip remote FetchContent updates after initial clone" ON)

# Pull ixwebsocket from source so builds remain portable across distros.
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ${YTBLND_FETCHCONTENT_UPDATES_DISCONNECTED})
set(FETCHCONTENT_QUIET OFF)

# Core sources shared between the main executable and tests (no UI, no main)
file(GLOB_RECURSE YTBLND_CORE_SOURCES
    AlgorithmLayer/*.cpp
    AppLayer/*.cpp
    AppInfrastructure/*.cpp
    ModelLayer/*.cpp
    ServiceLayer/*.cpp
)

# UI sources only compiled into the main executable
file(GLOB_RECURSE YTBLND_UI_SOURCES
    CONFIGURE_DEPENDS
    UILayer/*.cpp
)

# ── Gumbo (optional — HTML parser used by HtmlParser.cpp) ────────────────────
set(YTBLND_HAS_GUMBO FALSE)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GUMBO QUIET IMPORTED_TARGET gumbo)
    if(GUMBO_FOUND)
        set(YTBLND_HAS_GUMBO TRUE)
    endif()
else()
    message(STATUS "pkg-config not found - Gumbo HTML parsing disabled")
endif()

if(NOT YTBLND_HAS_GUMBO)
    message(STATUS "Gumbo not found — HtmlParser.cpp and HtmlSource.cpp excluded")
    list(REMOVE_ITEM YTBLND_CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure/HtmlParser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure/HtmlSource.cpp"
    )
endif()

# ── wxWidgets ─────────────────────────────────────────────────────────────────
# Support both package-config exports (common with vcpkg/homebrew) and
# CMake's FindwxWidgets module (common on many Linux distros).
set(YTBLND_HAS_WX FALSE)
set(YTBLND_WX_USE_FIND_MODULE FALSE)

find_package(wxWidgets QUIET CONFIG COMPONENTS core base net)
if(wxWidgets_FOUND)
    set(YTBLND_HAS_WX TRUE)
else()
    find_package(wxWidgets QUIET COMPONENTS core base net)
    if(wxWidgets_FOUND)
        include(${wxWidgets_USE_FILE})
        set(YTBLND_HAS_WX TRUE)
        set(YTBLND_WX_USE_FIND_MODULE TRUE)
    endif()
endif()

if(NOT YTBLND_HAS_WX)
    if(YTBLND_REQUIRE_WX)
        message(FATAL_ERROR "wxWidgets not found. Install wxWidgets and/or set CMAKE_PREFIX_PATH (or wxWidgets_ROOT) so CMake can discover it.")
    else()
        message(STATUS "wxWidgets not found - skipping UI executable and wx-dependent tests")
    endif()
endif()

# ── Main executable ──────────────────────────────────────────────────────────
# main.cpp is at YTBLND_src root — added explicitly so it is NOT picked up by
# the test GLOB and does not conflict with GTest's own main.
set(YTBLND_HAS_SQLITE FALSE)
if(YTBLND_ENABLE_SQLITE)
    find_package(SQLite3 QUIET)
    if(SQLite3_FOUND)
        set(YTBLND_HAS_SQLITE TRUE)
    else()
        message(STATUS "SQLite3 not found - SQLite-backed local persistence disabled")
    endif()
else()
    message(STATUS "SQLite support disabled via YTBLND_ENABLE_SQLITE=OFF")
endif()

if(NOT YTBLND_HAS_SQLITE)
    list(REMOVE_ITEM YTBLND_CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer/SqliteDataManager.cpp"
    )
endif()

find_package(CURL REQUIRED)

set(IXWEBSOCKET_BUILD_TEST OFF CACHE BOOL "Disable ixwebsocket tests" FORCE)
set(IXWEBSOCKET_USE_TLS ON CACHE BOOL "Enable TLS support in ixwebsocket" FORCE)

FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "Disable nlohmann/json tests" FORCE)

FetchContent_MakeAvailable(ixwebsocket nlohmann_json)

if(YTBLND_HAS_WX)
    add_executable(ytblnd
        ${YTBLND_CORE_SOURCES}
        ${YTBLND_UI_SOURCES}
        main.cpp
    )

    target_include_directories(ytblnd PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
    )

    target_link_libraries(ytblnd PRIVATE
        CURL::libcurl
        ixwebsocket::ixwebsocket
        nlohmann_json::nlohmann_json
    )

    if(YTBLND_HAS_GUMBO)
        target_link_libraries(ytblnd PRIVATE PkgConfig::GUMBO)
    endif()

    if(YTBLND_HAS_SQLITE)
        target_link_libraries(ytblnd PRIVATE SQLite::SQLite3)
    endif()

    if(YTBLND_WX_USE_FIND_MODULE)
        target_include_directories(ytblnd PRIVATE ${wxWidgets_INCLUDE_DIRS})
        target_compile_definitions(ytblnd PRIVATE ${wxWidgets_DEFINITIONS})
        target_link_libraries(ytblnd PRIVATE ${wxWidgets_LIBRARIES})
    else()
        target_link_libraries(ytblnd PRIVATE wx::core wx::base wx::net)
    endif()

    ytblnd_generate_linux_desktop_launcher(
        TARGET ytblnd
        TEMPLATE ${CMAKE_SOURCE_DIR}/resources/linux/ytblnd.desktop.in
        OUTPUT ${CMAKE_BINARY_DIR}/linux/ytblnd.desktop
        ICON_PATH ${CMAKE_SOURCE_DIR}/YTBLND_src/resources/ytblnd-framev2.png
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        APP_NAME YTBLND
        COMMENT "Blend YouTube Watch Later data into a shared feed"
        WM_CLASS ytblnd
        CUSTOM_TARGET_NAME ytblnd_linux_desktop_entry
    )

    ytblnd_generate_linux_desktop_launcher(
        TARGET ytblnd
        TEMPLATE ${CMAKE_SOURCE_DIR}/resources/linux/ytblnd.desktop.in
        OUTPUT ${CMAKE_BINARY_DIR}/linux/ytblnd-install.desktop
        EXEC_PATH ${CMAKE_INSTALL_PREFIX}/bin/ytblnd
        ICON_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps/ytblnd.png
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
        APP_NAME YTBLND
        COMMENT "Blend YouTube Watch Later data into a shared feed"
        WM_CLASS ytblnd
        CUSTOM_TARGET_NAME ytblnd_linux_install_desktop_entry
    )

    install(TARGETS ytblnd RUNTIME DESTINATION bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/YTBLND_src/resources/
        DESTINATION share/ytblnd/resources)
    install(FILES ${CMAKE_SOURCE_DIR}/YTBLND_src/resources/ytblnd-framev2.png
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME ytblnd.png)
    install(FILES ${CMAKE_BINARY_DIR}/linux/ytblnd-install.desktop
        DESTINATION share/applications
        RENAME ytblnd.desktop)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(YTBLND_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
    set(YTBLND_UI_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/Test_BlendCreationPanel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/Test_ThemeSwitching.cpp
    )
    set(YTBLND_SQLITE_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/Test_Auth.cpp
    )

    if(NOT (YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX))
        list(REMOVE_ITEM TEST_SOURCES ${YTBLND_UI_TEST_SOURCES})
    endif()

    if(NOT YTBLND_HAS_SQLITE)
        message(STATUS "SQLite disabled: excluding SQLite-dependent tests")
        list(REMOVE_ITEM TEST_SOURCES ${YTBLND_SQLITE_TEST_SOURCES})
    endif()

    add_executable(ytblnd_tests ${TEST_SOURCES})

    target_include_directories(ytblnd_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
        ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
    )

    if(YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX)
        target_include_directories(ytblnd_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
        )
    endif()

    target_compile_definitions(ytblnd_tests PRIVATE
        YTBLND_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        TESTING_MODE
    )

    # Core tests compile against non-UI sources by default.
    target_sources(ytblnd_tests PRIVATE
        ${YTBLND_CORE_SOURCES}
    )

    if(YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX)
        target_sources(ytblnd_tests PRIVATE
            ${YTBLND_UI_SOURCES}
        )
    endif()

    target_link_libraries(ytblnd_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        CURL::libcurl
        ixwebsocket::ixwebsocket
        nlohmann_json::nlohmann_json
    )

    if(YTBLND_HAS_GUMBO)
        target_link_libraries(ytblnd_tests PRIVATE PkgConfig::GUMBO)
    endif()

    if(YTBLND_HAS_SQLITE)
        target_link_libraries(ytblnd_tests PRIVATE SQLite::SQLite3)
    endif()

    if(YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX)
        if(YTBLND_WX_USE_FIND_MODULE)
            target_include_directories(ytblnd_tests PRIVATE ${wxWidgets_INCLUDE_DIRS})
            target_compile_definitions(ytblnd_tests PRIVATE ${wxWidgets_DEFINITIONS})
            target_link_libraries(ytblnd_tests PRIVATE ${wxWidgets_LIBRARIES})
        else()
            target_link_libraries(ytblnd_tests PRIVATE wx::core wx::base wx::net)
        endif()
    endif()

    add_test(NAME ytblnd_tests COMMAND ytblnd_tests)
endif()
