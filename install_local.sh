#!/bin/bash

set -e

# Install prefix defaults to user-local so no sudo is required.
prefix="${1:-$HOME/.local}"

# Configure CMake for this prefix (also regenerates desktop launcher metadata).
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$prefix"
# Build application targets.
cmake --build build
# Install binaries, resources, icon, and desktop entry into the prefix.
cmake --install build
# Refresh launcher database when available.
update-desktop-database "$prefix/share/applications" 2>/dev/null || true
# Refresh icon cache when available.
gtk-update-icon-cache "$prefix/share/icons/hicolor" 2>/dev/null || true

echo "Installed YTBLND to $prefix"