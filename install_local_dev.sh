#!/bin/bash

set -euo pipefail

prefix="${1:-$HOME/.local}"
docs_mode="${YTBLND_DEV_BUILD_DOCS:-auto}"

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists gumbo; then
	echo "Gumbo detected: installed dev build will include HTML parsing support."
else
	echo "Gumbo not detected: installed dev build will run without HTML parsing support."
fi

has_doxygen=0
if command -v doxygen >/dev/null 2>&1; then
	has_doxygen=1
	echo "Doxygen detected: dev docs target is available."
else
	echo "Doxygen not detected: skipping docs generation in dev install."
fi

# Dev install keeps SQLite-enabled code paths available for local testing.
cmake -S . -B build-dev -DCMAKE_INSTALL_PREFIX="$prefix" -DYTBLND_BUILD_TESTS=ON -DYTBLND_ENABLE_SQLITE=ON
cmake --build build-dev

if [[ "$docs_mode" == "1" || "$docs_mode" == "true" || "$docs_mode" == "on" ]]; then
	if [[ "$has_doxygen" -eq 0 ]]; then
		echo "YTBLND_DEV_BUILD_DOCS requested docs, but doxygen is not installed."
		exit 1
	fi
	cmake --build build-dev --target doc
elif [[ "$docs_mode" == "auto" && "$has_doxygen" -eq 1 ]]; then
	cmake --build build-dev --target doc
fi

cmake --install build-dev
update-desktop-database "$prefix/share/applications" 2>/dev/null || true
gtk-update-icon-cache "$prefix/share/icons/hicolor" 2>/dev/null || true

echo "Installed YTBLND dev build to $prefix"