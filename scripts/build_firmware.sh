#!/usr/bin/env bash
#
# Build firmware, optionally clear the `build/` directory and/or flash the
# binary to the Industruino. The Industruino bootloader must be active for this
# to work.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/setup.sh"

flash=false
port=""

# Parse options
while getopts "xp:" opt; do
  case "$opt" in
  x)
    rm -rf "$FIRMWARE_DIR/build"
    ;;
  p)
    port="$OPTARG"
    flash=true
    ;;
  *)
    echo "Usage: $0 [-x] [-p port]"
    echo "  -x       delete build directory before building"
    echo "  -p port  flash to Industruino via port after build"
    exit 1
    ;;
  esac
done

# Build firmware and optimize for speed
cd "$FIRMWARE_DIR"

cmake --preset samd21g18a-release
cmake --build --preset samd21g18a-release

# Symlink compile commands for `clangd`.
ln -sfn build/samd21g18a-release/compile_commands.json compile_commands.json

if "$flash"; then
  bossac \
    --debug \
    --port="$port" \
    --offset=0x4000 \
    --erase \
    --write \
    --verify \
    --reset \
    build/samd21g18a-release/firmware.bin
fi
