# CMake generated Testfile for 
# Source directory: /home/slipnut44/git_repo/group45/YTBLND_src
# Build directory: /home/slipnut44/git_repo/group45/build-dev/YTBLND_src
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ytblnd_tests "/home/slipnut44/git_repo/group45/build-dev/YTBLND_src/ytblnd_tests")
set_tests_properties(ytblnd_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/slipnut44/git_repo/group45/YTBLND_src/CMakeLists.txt;277;add_test;/home/slipnut44/git_repo/group45/YTBLND_src/CMakeLists.txt;0;")
subdirs("../_deps/ixwebsocket-build")
subdirs("../_deps/nlohmann_json-build")
