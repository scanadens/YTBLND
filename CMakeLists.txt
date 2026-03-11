# For vscode environments to pick up on external dependencies
cmake_minimum_required(VERSION 3.16)
project(YTBLND)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(YTBLND_src)