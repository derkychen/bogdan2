"""This module contains the Bogdan 2 CLI entry point."""

import argparse
import json
from pathlib import Path

from host.profiler import Profiler


class InvalidInstructionJSON(Exception):
    """When an invalid JSON is in the instruction file."""


parser = argparse.ArgumentParser()
subparsers = parser.add_subparsers(dest="command", required=True)

calibrate_command = subparsers.add_parser(
    "calibrate", help="Calibrate the profiler to the beam based on intensity."
)

profile_command = subparsers.add_parser(
    "profile", help="Run the beam profiler based on your instructions."
)
profile_command.add_argument(
    "port", help="Port over which to communicate to the microcontroller."
)
profile_command.add_argument(
    "instruction", help="Required path to instruction JSON."
)


def main():
    """Parse arguments and call the correct functions."""
    args = parser.parse_args()

    # Lazily instantiate `Profiler` to minimize unneeded work on invalid input.
    if args.command == "calibrate":
        Profiler().calibrate()

    elif args.command == "profile":
        port = args.port
        instruction_path = Path(args.instruction)

        if instruction_path.is_file():
            try:
                with open(instruction_path, encoding="utf-8") as f:
                    Profiler().profile(port, json.load(f))

            except json.JSONDecodeError as e:
                raise InvalidInstructionJSON(
                    f"Invalid JSON in '{instruction_path}': {e}"
                ) from e
        else:
            raise FileNotFoundError("Instruction file not found.")
