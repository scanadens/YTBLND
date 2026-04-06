#!/bin/bash
# Runs only the live backend connectivity gtests from the dev/test build.

set -euo pipefail

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gumbo; then
	echo "Gumbo detected: HTML parsing support is enabled in this build."
else
	echo "Gumbo not detected: HTML parsing support is disabled in this build."
fi

echo "Doxygen is not required for live backend connectivity tests."

test_binary="./build/YTBLND_src/ytblnd_tests"

if [[ ! -x "$test_binary" ]]; then
	echo "Dev test binary not found at $test_binary"
	echo "Run ./build_dev_src.sh first (single build dir mode). This test build expects SQLite and GoogleTest to be installed."
	exit 1
fi

# allow single gtest flags to be passed on the gtest binary for further testing
YTBLND_RUN_LIVE_BACKEND_TESTS=1 \
"$test_binary" --gtest_filter='LiveBackendConnectivityTest.*' "$@"
