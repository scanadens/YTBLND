#!/bin/bash

set -euo pipefail

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gumbo; then
	echo "Gumbo detected: HTML parsing support will be enabled."
else
	echo "Gumbo not detected: HTML parsing support will be disabled (CSV import still works)."
fi

if command -v doxygen >/dev/null 2>&1; then
	echo "Doxygen detected: optional docs can be generated separately."
else
	echo "Doxygen not detected: no impact on user build/install/run paths."
fi

# Default app build: no test-only dependencies and no SQLite requirement.
cmake -S . -B build -DYTBLND_BUILD_TESTS=OFF -DYTBLND_ENABLE_SQLITE=OFF
cmake --build build
