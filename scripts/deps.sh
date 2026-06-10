#!/usr/bin/env bash
#
# Install relevant dependencies

set -euo pipefail

readonly ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || {
  echo "Error: must be run inside the Git repository" >&2
  exit 1
}

mkdir -p "$HOME/.pico"

# Install and configure Pico SDK
if [ ! -d "$HOME/.pico/pico-sdk" ]; then
  git clone https://github.com/raspberrypi/pico-sdk.git "$HOME/.pico/pico-sdk"
fi

git -C "$HOME/.pico/pico-sdk" submodule update --init

# Dependencies for Python virtual environment
cd "$ROOT"
uv venv
uv sync
