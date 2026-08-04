#!/usr/bin/env bash
#
# CI checks.

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/vars.sh"

clang-format --dry-run -Werror $(git ls-files "$FIRMWARE_DIR/**/*.[ch]" | grep -v external)

(cd host && ruff check . && ruff format --check . && basedpyright)

cmake --build --preset samd21g18a-release && cmake --build --preset samd21g18a-debug
