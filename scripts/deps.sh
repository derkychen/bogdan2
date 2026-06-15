#!/usr/bin/env bash
#
# Install and update dependencies of the project.

set -euo pipefail

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

readonly PROJECT_DIR="$(git rev-parse --show-toplevel 2>/dev/null)" || {
  echo "Error: This script must be run from inside a Git repository." >&2
  exit 1
}
readonly FIRMWARE_DIR="$PROJECT_DIR/firmware"
readonly EXTERNAL_DIR="$FIRMWARE_DIR/external"

# Commands that should be installed as per the README.
readonly REQUIRED_CMDS=(
  git
  cmake
  ninja
  arm-none-eabi-gcc
  arm-none-eabi-objcopy
  arm-none-eabi-size
  cpackget
  uv
)

# Each 'spec' for the required git submodules are formatted as:
# `name URL path ref`
readonly TINYUSB="tinyusb \
  https://github.com/hathach/tinyusb.git \
  firmware/external/tinyusb \
  master"
readonly CMSIS="cmsis \
  https://github.com/ARM-software/CMSIS_6.git \
  firmware/external/cmsis \
  v6.1.0"
readonly JSMN="jsmn \
  https://github.com/zserge/jsmn.git \
  firmware/external/jsmn \
  master"
readonly INDUSTRUINOSAMD="IndustruinoSAMD \
  https://github.com/Industruino/IndustruinoSAMD.git \
  firmware/external/IndustruinoSAMD \
  master"
readonly GIT_SUBMODULES=(
  "$TINYUSB"
  "$CMSIS"
  "$JSMN"
  "$INDUSTRUINOSAMD"
)

# CMSIS Pack for SAMD21
if [ -n "${CMSIS_PACK_ROOT:-}" ]; then
  readonly CMSIS_PACK_ROOT
else
  case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*)
    if [ -n "${LOCALAPPDATA:-}" ] && command -v cygpath >/dev/null 2>&1; then
      readonly CMSIS_PACK_ROOT="$(cygpath -u "$LOCALAPPDATA")/Arm/Packs"
    elif [ -n "${LOCALAPPDATA:-}" ]; then
      readonly CMSIS_PACK_ROOT="$LOCALAPPDATA/Arm/Packs"
    else
      readonly CMSIS_PACK_ROOT="$HOME/AppData/Local/Arm/Packs"
    fi
    ;;
  *)
    readonly CMSIS_PACK_ROOT="$HOME/.cache/arm/packs"
    ;;
  esac
fi

readonly SAMD21_DFP_PACK="Microchip::SAMD21_DFP@3.8.270"

# Check that all required commands are executable
for cmd in "${REQUIRED_CMDS[@]}"; do
  require_cmd "$cmd"
done

mkdir -p "$EXTERNAL_DIR"
mkdir -p "$CMSIS_PACK_ROOT"

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

git submodule update --init --recursive

export CMSIS_PACK_ROOT

echo "Installing CMSIS-Pack dependencies into: $CMSIS_PACK_ROOT"
cpackget add "$SAMD21_DFP_PACK" || {
  echo "Warning: cpackget add failed." >&2
  exit 1
}

echo "Important SAMD21_DFP files:"
find "$CMSIS_PACK_ROOT" -name sam.h
find "$CMSIS_PACK_ROOT" -name startup_samd21.c
find "$CMSIS_PACK_ROOT" -name system_samd21.c

# Dependencies for Python virtual environment
cd "$PROJECT_DIR"

uv venv
uv sync
