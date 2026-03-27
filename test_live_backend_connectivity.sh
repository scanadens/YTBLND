#!/bin/bash
# runs only the live backend gtest. assuming the backend is live and running

YTBLND_RUN_LIVE_BACKEND_TESTS=1 ./build/YTBLND_src/ytblnd_tests --gtest_filter='LiveBackendConnectivityTest.*'
