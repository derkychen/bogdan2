#!/usr/bin/env bash
#
# Optionally build firmware and/or clear the `build/` directory and/or flash the
# binary to the Industruino (this only works if the bootloader is active).

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/vars.sh"

usage() {
  cat <<EOF
Usage: $0 -p PRESET [-x] [-b] [-c PORT]
  -p PRESET  Preset of the firmware. Must be 'debug' or 'release'.
  -x         Clean build directory.
  -b         Build the firmware.
  -c PORT    Flash the firmware to Industruino IND.I/O via a port.
EOF
}

if (($# == 0)); then
  usage
  exit 1
fi

PRESET=''
CLEAN=false
BUILD=false
PORT=''

while getopts 'xbp:c:' opt; do
  case "$opt" in
  p)
    PRESET="$OPTARG"
    ;;
  x)
    CLEAN=true
    ;;
  b)
    BUILD=true
    ;;
  c)
    PORT="$OPTARG"
    ;;
  *)
    usage
    exit 1
    ;;
  esac
done

shift "$((OPTIND - 1))"

if (($# > 0)); then
  printf 'Error: Unexpected argument: %s\n' "$1" >&2
  usage >&2
  exit 1
fi

# Check preset validity.
if [[ "$PRESET" != 'debug' && "$PRESET" != 'release' ]]; then
  printf 'Error: Invalid preset: %s\n' "$PRESET" >&2
  usage >&2
  exit 1
fi

readonly PRESET_BUILD_DIR="$FIRMWARE_DIR/build/samd21g18a-$PRESET"

# Check that an action was provided.
if [[ "$CLEAN" == false && "$BUILD" == false && "$PORT" == '' ]]; then
  printf 'Error: No actions were provided.' >&2
  usage >&2
  exit 1
fi

# Optionally clean the build directory.
if [[ "$CLEAN" == true ]]; then
  rm -rf "$PRESET_BUILD_DIR"
fi

# Optionally build firmware for debugging or release; symlink compile commands
# for `clangd`.
if [[ "$BUILD" == true ]]; then
  readonly CMAKE_PRESET="samd21g18a-$PRESET"

  cmake -S "$FIRMWARE_DIR" --preset "$CMAKE_PRESET"
  cmake --build "$PRESET_BUILD_DIR"

  ln -sfn "$PRESET_BUILD_DIR"/compile_commands.json "$FIRMWARE_DIR"/compile_commands.json
fi

# Optionally flash firmware to a port.
if [[ -n "$PORT" ]]; then
  readonly BINARY_PATH="$PRESET_BUILD_DIR/firmware.bin"

  if [[ ! -f "$BINARY_PATH" ]]; then
    printf 'Error: Binary does not exist: %s\n' "$BINARY_PATH" >&2
    exit 1
  fi

  bossac \
    --debug \
    --port="$PORT" \
    --offset=0x4000 \
    --erase \
    --write \
    --verify \
    --reset \
    "$BINARY_PATH"
fi
