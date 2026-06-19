#!/usr/bin/env bash
#
# Build firmware, optionally clear the `build/` directory and/or flash the
# binary to the Industruino.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/setup.sh"

flash=false

# Parse options
while getopts "xf" opt; do
  case "$opt" in
  x)
    rm -rf "$FIRMWARE_DIR/build"
    ;;
  f)
    flash=true
    ;;
  *)
    echo "Usage: $0 [-x] [-u]"
    echo "  -x  delete build directory before building"
    echo "  -f  flash to Industruino after build"
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

# TODO: USB CDC flashing instead of manually pressing reset button.
if "$flash"; then
  bossac \
    -d \
    --port=/dev/cu.usbmodem1101 \
    -o 0x4000 \
    -e \
    -w \
    -v \
    -b \
    -R \
    build/samd21g18a-release/firmware.bin
fi
