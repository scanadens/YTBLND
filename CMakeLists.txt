cmake_minimum_required(VERSION 3.16)
project(YTBLND)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build the main project + tests
add_subdirectory(YTBLND_src)

# Doxygen documentation target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found. Documentation generation disabled.")
    add_custom_target(doc
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not installed. Install with: sudo apt install doxygen (Ubuntu/Debian) or brew install doxygen (macOS)"
    )
endif()