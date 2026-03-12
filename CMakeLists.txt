cmake_minimum_required(VERSION 3.16)
project(YTBLND)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build the main project + tests
add_subdirectory(YTBLND_src)

# Now that ytblnd exists, link SQLite
find_package(SQLite3 REQUIRED)
target_link_libraries(ytblnd PRIVATE SQLite::SQLite3)