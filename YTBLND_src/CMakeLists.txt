# more stuff for vscode to know how to build the code
# YTBLND_src/CMakeLists.txt

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

# --- Include directories for your headers ---
target_include_directories(ytblnd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
)

# --- Link Gumbo (based on my fedora system, not sure if this needs to be changed later on) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO gumbo)  # not REQUIRED so tests still build without it

if(GUMBO_FOUND)
    target_include_directories(ytblnd PRIVATE ${GUMBO_INCLUDE_DIRS})
    target_link_libraries(ytblnd PRIVATE ${GUMBO_LIBRARIES})
endif()

# --- GOOGLE TEST SETUP ---

# Enable testing
enable_testing()

# Find gtest (Fedora installs it as gtest-devel)
find_package(GTest REQUIRED)

# Gather test sources
file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

# Create test executable
add_executable(ytblnd_tests ${TEST_SOURCES})

# Include your project headers in tests
target_include_directories(ytblnd_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add the same include directories so tests can find all headers
target_include_directories(ytblnd_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/AlgorithmLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/AppInfrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/ModelLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ServiceLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/UILayer
)

# Expose the source root so tests can locate fixture files by absolute path
target_compile_definitions(ytblnd_tests PRIVATE
    YTBLND_SRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Link gtest + your project code
target_link_libraries(ytblnd_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Compile project source files into the test target so tests can use them
target_sources(ytblnd_tests PRIVATE ${YTBLND_SOURCES})

# Register tests with CTest
add_test(NAME ytblnd_tests COMMAND ytblnd_tests)

