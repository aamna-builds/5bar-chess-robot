#!/usr/bin/env bash
# Build and run the firmware's PC-side unit tests (kinematics + protocol).
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
    firmware/test/test_native.cpp firmware/fivebar_chess/kinematics.cpp firmware/fivebar_chess/protocol.cpp \
    -o build/test_native
./build/test_native matlab/output/square_table.csv
