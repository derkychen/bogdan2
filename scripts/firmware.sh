#!/usr/bin/env bash
#
# Optionally build firmware and/or clear the `build/` directory and/or flash the
# binary to the Industruino (this only works if the bootloader is active).

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/setup.sh"

usage() {
  cat <<EOF
Usage: $0 [-x] [-b] [-p port]
  -x       delete build directory
  -b       build the firmware
  -p PORT  flash to Industruino via port after build
EOF
}

if (($# == 0)); then
  usage
  exit 1
fi

while getopts "bxp:" opt; do
  case "$opt" in
  x)
    rm -rf "$FIRMWARE_DIR/build"
    ;;
  b)
    # Build firmware and optimize for speed
    cd "$FIRMWARE_DIR"

    cmake --preset samd21g18a-release
    cmake --build --preset samd21g18a-release

    # Symlink compile commands for `clangd`.
    ln -sfn build/samd21g18a-release/compile_commands.json compile_commands.json
    ;;
  p)
    bossac \
      --debug \
      --port="$OPTARG" \
      --offset=0x4000 \
      --erase \
      --write \
      --verify \
      --reset \
      "$FIRMWARE_DIR/build/samd21g18a-release/firmware.bin"
    ;;
  *)
    usage
    exit 1
    ;;
  esac
done
