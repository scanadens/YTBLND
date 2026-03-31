#!/bin/bash
# Runs only live backend gtests with a quick health preflight.

set -euo pipefail

HTTP_BASE_URL="${YTBLND_LIVE_BACKEND_HTTP_BASE_URL:-http://localhost:8080}"

if ! curl -sS -f "${HTTP_BASE_URL}/ping" >/dev/null; then
	echo "Live backend is unreachable at ${HTTP_BASE_URL}."
	echo "Start the backend first or set YTBLND_LIVE_BACKEND_HTTP_BASE_URL to the correct host."
	exit 1
fi

YTBLND_RUN_LIVE_BACKEND_TESTS=1 \
./build/YTBLND_src/ytblnd_tests --gtest_filter='LiveBackendConnectivityTest.*'
