#!/usr/bin/env bash
#
# Set some variables used by other scripts.
#
# NOTE: This script should not be executed directly. It should only be sourced
# by other scripts.

set -euo pipefail

readonly PROJECT_DIR="$(git rev-parse --show-toplevel 2>/dev/null)" || {
  echo "Error: This script must be run from inside a Git repository." >&2
  exit 1
}
readonly FIRMWARE_DIR="$PROJECT_DIR/firmware"
readonly EXTERNAL_DIR="$FIRMWARE_DIR/external"
