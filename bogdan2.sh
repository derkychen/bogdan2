#!/usr/bin/env bash
#
# Run the main Bogdan 2 scripts.

set -euo pipefail

source .venv/Scripts/activate

COMMAND="${1:-}"

case "$COMMAND" in
calibrate)
  clear
  python calibrate.py
  ;;

profile)
  GRID_FILE="${2:-}"
  if [ -z "$GRID_FILE" ]; then
    echo "Path to instruction must be specified."
    echo
    echo "Usage:"
    echo "  ./bogdan2.sh profile path/to/instruction.json>"
    exit 1
  fi

  python profile.py "$GRID_FILE"
  ;;

*)
  echo "Unknown command: $COMMAND"
  echo
  echo "Usage:"
  echo "  ./bogdan2.sh calibrate"
  echo "  ./bogdan2.sh profile path/to/instruction.json>"
  exit 1
  ;;
esac
