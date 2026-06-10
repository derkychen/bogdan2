#!/usr/bin/env bash
#
# Build firmware, optionally clear the `build/` directory or copy the UF2 file to the Pico.

set -euo pipefail

readonly ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || {
  echo "Error: must be run inside the Git repository" >&2
  exit 1
}

cd "$ROOT"

export PICO_SDK_PATH="${PICO_SDK_PATH:-$HOME/.pico/pico-sdk}"

upload=false

# Parse options
while getopts "xu" opt; do
  case "$opt" in
  x)
    rm -rf firmware/build
    ;;
  u)
    upload=true
    ;;
  *)
    echo "Usage: $0 [-x] [-u]"
    echo "  -x  delete build directory before building"
    echo "  -u  upload/copy UF2 after build"
    exit 1
    ;;
  esac
done

cd firmware

# TODO: Fix for compatibility with Windows
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bogdan2

if "$upload"; then
  cp build/bogdan2.uf2 /e/ # Git Bash in Windows
  # cp build/bogdan2.uf2 /media/$USER/RP2350 # Linux
  # cp build/bogdan2.uf2 /Volumes/RP2350   # macOS
fi
