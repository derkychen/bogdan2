#!/bin/bash
#
# Build and copy the UF2 to the Pico.

set -euo pipefail

FLAG="${1:-}"

export PICO_SDK_PATH="${PICO_SDK_PATH:-$HOME/.pico/pico-sdk}"

cd firmware

if [[ "$FLAG" == "--clear" ]]; then
  rm -rf build
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target bogdan2

# cp build/bogdan2.uf2 /RPI-RP2/bogdan2.uf2 # TODO: Verify location of Pico
