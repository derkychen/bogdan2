"""This module contains the Bogdan 2 CLI entry point."""

import argparse
from pathlib import Path

from profiler import Profiler

parser = argparse.ArgumentParser()
subparsers = argparse.add_subparsers(dest="command", required=True)

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
    profiler = Profiler()

    if args.command == "calibrate":
        profiler.calibrate()

    elif args.command == "profile":
        port = Path(args.port)
        instruction_path = Path(args.instruction)

        if instruction_path.is_file():
            profiler.profile(port, instruction_path)
        else:
            raise FileNotFoundError("Instruction file not found.")
