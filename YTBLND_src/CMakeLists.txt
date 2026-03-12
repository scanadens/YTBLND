# --- Collect all source files from your layers ---
file(GLOB_RECURSE YTBLND_SOURCES
    AlgorithmLayer/*.cpp
    AppLayer/*.cpp
    AppInfrastructure/*.cpp
    ModelLayer/*.cpp
    ServiceLayer/*.cpp
    UILayer/*.cpp
)

# --- Create the main executable ---
add_executable(ytblnd ${YTBLND_SOURCES})

# Include directories for the main executable
target_include_directories(ytblnd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
)

# --- Collect all source files from your layers ---
enable_testing()

# Prefer modern CMake GTest discovery
find_package(GTest REQUIRED)
find_package(SQLite3 REQUIRED)

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

# Expose the source root so tests can locate fixture files
target_compile_definitions(ytblnd_tests PRIVATE
    YTBLND_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Link GTest + SQLite + your project code
target_link_libraries(ytblnd_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    SQLite::SQLite3
)

# Compile project source files into the test target
target_sources(ytblnd_tests PRIVATE ${YTBLND_SOURCES})

add_test(NAME ytblnd_tests COMMAND ytblnd_tests)
