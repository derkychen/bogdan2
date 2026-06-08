#!/bin/bash
#
# Install relevant dependencies

set -euo pipefail

cd $HOME

# Install and configure Pico SDK
mkdir ".pico" && cd "$_"

git clone https://github.com/raspberrypi/pico-sdk.git
git submodule update --init

# Dependencies for Python virtual environment
cd $(dirname "${BASH_SOURCE[0]}")
uv venv
uv sync
