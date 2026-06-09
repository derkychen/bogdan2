#!/usr/bin/bash
#
# Build and copy the UF2 to the Pico.

set -euo pipefail

OPTION="${1:-}"

export PICO_SDK_PATH="${PICO_SDK_PATH:-$HOME/.pico/pico-sdk}"

cd firmware

# Optionally clear the `build/` directory
if [[ "$OPTION" == "--clear-build" ]]; then
  rm -rf build
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bogdan2

cp build/bogdan2.uf2 /e/ # On Windows
# cp build/bogdan2.uf2 /Volumes/RP2350 # On macOS/Linux
