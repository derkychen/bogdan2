#!/usr/bin/env bash
#
# Set some variables used by other scripts.
#
# NOTE: This script should not be executed directly. It should only be sourced
# by other scripts.

set -euo pipefail

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  echo "Error: This script should not be executed directly." >&2
  exit 1
fi

PROJECT_DIR="$(git rev-parse --show-toplevel 2>/dev/null)" || {
  echo "Error: This script must be run from inside a Git repository." >&2
  return 1
}

readonly PROJECT_DIR
readonly FIRMWARE_DIR="$PROJECT_DIR/firmware"
readonly HOST_DIR="$PROJECT_DIR/host"
readonly SCRIPTS_DIR="$PROJECT_DIR/scripts"
