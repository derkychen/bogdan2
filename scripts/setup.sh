#!/usr/bin/env bash
#
# Synchronize dependencies of the project.

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/vars.sh"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Error: Missing required command: $1" >&2
    exit 1
  }
}

# Commands that should be installed as per the README.
readonly REQUIRED_CMDS=(
  arm-none-eabi-gcc
  arm-none-eabi-objcopy
  arm-none-eabi-size
  bossac
  cmake
  git
  ninja
  uv
)

# Check that all required commands are executable
for cmd in "${REQUIRED_CMDS[@]}"; do
  require_cmd "$cmd"
done

# Git hooks.
git -C "$PROJECT_DIR" config core.hooksPath .githooks

# Dependencies for Python virtual environment
uv sync --project "$HOST_DIR" --locked
