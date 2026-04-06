#!/bin/bash

set -euo pipefail

docs_mode="${YTBLND_DEV_BUILD_DOCS:-auto}"

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gumbo; then
	echo "Gumbo detected: HTML parsing support will be enabled for dev build."
else
	echo "Gumbo not detected: HTML parsing support will be disabled for dev build."
fi

has_doxygen=0
if command -v doxygen >/dev/null 2>&1; then
	has_doxygen=1
	echo "Doxygen detected: dev docs target is available."
else
	echo "Doxygen not detected: skipping docs generation in dev build."
fi

# Dev/test build: enables SQLite-backed paths and test targets.
cmake -S . -B build -DYTBLND_BUILD_TESTS=ON -DYTBLND_ENABLE_SQLITE=ON
cmake --build build

if [[ "$docs_mode" == "1" || "$docs_mode" == "true" || "$docs_mode" == "on" ]]; then
	if [[ "$has_doxygen" -eq 0 ]]; then
		echo "YTBLND_DEV_BUILD_DOCS requested docs, but doxygen is not installed."
		exit 1
	fi
	cmake --build build --target doc
elif [[ "$docs_mode" == "auto" && "$has_doxygen" -eq 1 ]]; then
	cmake --build build --target doc
fi

echo "Dev build completed in build/"