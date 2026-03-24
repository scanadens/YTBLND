# ── Source globs ─────────────────────────────────────────────────────────────

option(YTBLND_BUILD_TESTS "Build unit tests" ON)
option(YTBLND_BUILD_UI_TESTS "Build wxWidgets/UI tests" OFF)
option(YTBLND_REQUIRE_WX "Fail configure if wxWidgets is missing" OFF)

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
    UILayer/*.cpp
)

# ── Gumbo (optional — HTML parser used by HtmlParser.cpp) ────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO gumbo)   # not REQUIRED — build still works without it

if(NOT GUMBO_FOUND)
    message(STATUS "Gumbo not found — HtmlParser.cpp and HtmlSource.cpp excluded")
    list(REMOVE_ITEM YTBLND_CORE_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure/HtmlParser.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure/HtmlSource.cpp"
    )
endif()

# ── wxWidgets ─────────────────────────────────────────────────────────────────
find_package(wxWidgets QUIET COMPONENTS core base net)

set(YTBLND_HAS_WX FALSE)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
    set(YTBLND_HAS_WX TRUE)
elseif(YTBLND_REQUIRE_WX)
    message(FATAL_ERROR "wxWidgets not found. Install wxWidgets or configure with -DYTBLND_REQUIRE_WX=OFF.")
else()
    message(STATUS "wxWidgets not found - skipping UI executable and wx-dependent tests")
endif()

# ── Main executable ──────────────────────────────────────────────────────────
# main.cpp is at YTBLND_src root — added explicitly so it is NOT picked up by
# the test GLOB and does not conflict with GTest's own main.
find_package(SQLite3 REQUIRED)
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
        SQLite::SQLite3
        ${wxWidgets_LIBRARIES}
    )
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(YTBLND_BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)

    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
    set(YTBLND_UI_TEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/Test_BlendCreationPanel.cpp
    )

    if(NOT (YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX))
        list(REMOVE_ITEM TEST_SOURCES ${YTBLND_UI_TEST_SOURCES})
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
        SQLite::SQLite3
    )

    if(YTBLND_BUILD_UI_TESTS AND YTBLND_HAS_WX)
        target_link_libraries(ytblnd_tests PRIVATE
            ${wxWidgets_LIBRARIES}
        )
    endif()

    add_test(NAME ytblnd_tests COMMAND ytblnd_tests)
endif()
