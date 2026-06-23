#!/usr/bin/env bash
#
# Install and update dependencies of the project.

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/setup.sh"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Error: Missing required command: $1" >&2
    exit 1
  }
}

add_git_submodule_if_not_installed() {
  local name="$1"
  local url="$2"
  local path="$3"
  local _="$4"

  if [ -f "$PROJECT_DIR/.gitmodules" ] &&
    git config --file "$PROJECT_DIR/.gitmodules" --get-regexp path 2>/dev/null |
    awk '{print $2}' |
      grep -qx "$path"; then
    echo "Submodule already registered: $name ($path)"
  elif git ls-files --error-unmatch "$path" >/dev/null 2>&1; then
    echo "Submodule path already exists in Git index: $name ($path)"
    echo "If this is a broken partial submodule, clean it with:"
    echo "  git rm --cached $path"
    echo "  rm -rf $path"
    echo "  rm -rf .git/modules/$path"
    exit 1
  else
    echo "Adding submodule: $name -> $path"
    git submodule add "$url" "$path"
  fi
}

# Commands that should be installed as per the README.
readonly REQUIRED_CMDS=(
  git
  cmake
  ninja
  arm-none-eabi-gcc
  arm-none-eabi-objcopy
  arm-none-eabi-size
  uv
)

# Each 'spec' for the required git submodules are formatted as:
# `name URL path ref`
readonly CMSIS="cmsis \
  https://github.com/ARM-software/CMSIS_6.git \
  firmware/external/cmsis \
  v6.1.0"
readonly CMSIS_ATMEL="cmsis-atmel \
  https://github.com/arduino/ArduinoModule-CMSIS-Atmel.git \
  firmware/external/cmsis-atmel \
  master"
readonly TINYUSB="tinyusb \
  https://github.com/hathach/tinyusb.git \
  firmware/external/tinyusb \
  master"
readonly JSMN="jsmn \
  https://github.com/zserge/jsmn.git \
  firmware/external/jsmn \
  master"

readonly GIT_SUBMODULES=(
  "$CMSIS"
  "$CMSIS_ATMEL"
  "$TINYUSB"
  "$JSMN"
)

# Check that all required commands are executable
for cmd in "${REQUIRED_CMDS[@]}"; do
  require_cmd "$cmd"
done

mkdir -p "$EXTERNAL_DIR"

# Dependencies for firmware
cd "$PROJECT_DIR"

for git_submodule in "${GIT_SUBMODULES[@]}"; do
  read -r name url path ref <<<"$git_submodule"
  add_git_submodule_if_not_installed "$name" "$url" "$path" "$ref"
done

git submodule update --init --recursive

for git_submodule in "${GIT_SUBMODULES[@]}"; do
  read -r name url path ref <<<"$git_submodule"

  echo "Updating $name to $ref"

  cd "$PROJECT_DIR/$path"
  git fetch --tags origin
  git checkout "$ref"
  cd "$PROJECT_DIR"
done

for git_submodule in "${GIT_SUBMODULES[@]}"; do
  read -r name url path ref <<<"$git_submodule"

  echo "Updating nested submodules for $name"

  cd "$PROJECT_DIR/$path"
  git submodule sync --recursive
  git submodule update --init --recursive
  cd "$PROJECT_DIR"
done

# Dependencies for Python virtual environment
cd "$PROJECT_DIR"

if [ ! -d "$PROJECT_DIR/.venv" ]; then
  uv venv
else
  echo "Python virtual environment already exists: $PROJECT_DIR/.venv"
fi

uv sync
