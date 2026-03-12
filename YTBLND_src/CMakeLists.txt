# ── Source globs ─────────────────────────────────────────────────────────────

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

# ── wxWidgets ─────────────────────────────────────────────────────────────────
find_package(wxWidgets 3 REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

# ── Main executable ──────────────────────────────────────────────────────────
# main.cpp is at YTBLND_src root — added explicitly so it is NOT picked up by
# the test GLOB and does not conflict with GTest's own main.
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

find_package(SQLite3 REQUIRED)
target_link_libraries(ytblnd PRIVATE
    SQLite::SQLite3
    ${wxWidgets_LIBRARIES}
)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
find_package(GTest REQUIRED)

file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

add_executable(ytblnd_tests ${TEST_SOURCES})

target_include_directories(ytblnd_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
)

target_compile_definitions(ytblnd_tests PRIVATE
    YTBLND_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Tests use core sources only — no UI, no wxWidgets dependency
target_sources(ytblnd_tests PRIVATE ${YTBLND_CORE_SOURCES})

target_link_libraries(ytblnd_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    SQLite::SQLite3
)

add_test(NAME ytblnd_tests COMMAND ytblnd_tests)
