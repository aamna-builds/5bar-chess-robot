#!/usr/bin/env bash
# Compile the sketch for an Arduino Uno with arduino-cli (recommended), and
# print the flash/RAM usage. Install once:
#   arduino-cli core install arduino:avr && arduino-cli lib install Servo
set -euo pipefail
cd "$(dirname "$0")/.."
arduino-cli compile --fqbn arduino:avr:uno --warnings all firmware/fivebar_chess "$@"
