# For vscode environments to pick up on external dependencies
cmake_minimum_required(VERSION 3.16)
project(YTBLND)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(YTBLND_src)

# --- config for test building ---
add_executable(test
	YTBLND_src/tests/Test_AppInfr.cpp
)

set_target_properties(test PROPERTIES OUTPUT_NAME "YTBLND_src_test.exe")

target_link_libraries(test
    PRIVATE
        gtest_main
		gtest
        pthread        # Linux only
)