#!/usr/bin/env bash
#
# Install relevant dependencies

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "$HOME/.pico"

# Install and configure Pico SDK
if [ ! -d "$HOME/.pico/pico-sdk" ]; then
  git clone https://github.com/raspberrypi/pico-sdk.git "$HOME/.pico/pico-sdk"
fi

git -C "$HOME/.pico/pico-sdk" submodule update --init

# Dependencies for Python virtual environment
cd "$SCRIPT_DIR"
uv venv
uv sync
