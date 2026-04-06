#!/bin/bash

set -euo pipefail

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gumbo; then
	echo "Gumbo detected: HTML parser paths are testable."
else
	echo "Gumbo not detected: HTML parser paths may be excluded from this dev test build."
fi

echo "Doxygen is not required for tests."

test_binary="./build-dev/YTBLND_src/ytblnd_tests"

if [[ ! -x "$test_binary" ]]; then
	echo "Dev test binary not found at $test_binary"
	echo "Run ./build_dev_src.sh first. This test build expects SQLite and GoogleTest to be installed."
	exit 1
fi

# forwards gtest flags to the testing binary
"$test_binary" "$@"
