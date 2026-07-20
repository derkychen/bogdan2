"""This module contains the main profiling functionality."""

import json
import os
import time
from typing import Final

import numpy as np
import serial

from host.pdxc2.constants import (
    ANALOG_IN_GAIN_0_TO_10,
    ANALOG_IN_OFFSET_0_TO_10,
    ANALOG_OUT_GAIN_0_TO_10,
    ANALOG_OUT_OFFSET_0_TO_10,
)
from host.pdxc2.controller import Controller
from host.pico.constants import (
    RANGE_5V,
    RANGE_20V,
    SCOPE_MODE_BULK,
    SCOPE_MODE_SINGLE,
)
from host.pico.scope import Scope
from host.utils.instruction import (
    MODE_CONTINUOUS,
    MODE_MAP,
    InvalidModeException,
    check_no_none,
    mcu_instruction,
)

X_PDXC2_SERIAL_NUM: Final[bytes] = b"112547939"
Y_PDXC2_SERIAL_NUM: Final[bytes] = b"112512664"

CALIBRATION_REFRESH_S: Final[float] = 0.100

BAUD_RATE: Final[int] = 115200


class InvalidInstructionJSONException:
    """When an invalid JSON is in the instruction file."""


class InvalidJSONFromMCUException:
    """When an invalid JSON is received from the microcontroller."""


def _clear_terminal() -> None:
    os.system("cls" if os.name == "nt" else "clear")


class Profiler:
    """Profiler class responsible for controlling scope and controllers."""

    def __init__(self) -> None:
        """Initialize a profiler instance."""
        self._scope = Scope()

        self._x_controller = Controller(X_PDXC2_SERIAL_NUM)
        self._y_controller = Controller(X_PDXC2_SERIAL_NUM)

        self._scope.open()
        self._scope.setup()

        self._scope.configure_channels(
            "trigger_mv",
            RANGE_5V,
            "x_mv",
            RANGE_20V,
            "y_mv",
            RANGE_20V,
            "intensity_mv",
            RANGE_20V,
        )

        self._x_controller.enable()
        self._y_controller.enable()

        self._x_controller.set_to_analog_rising_trigger_mode()
        self._y_controller.set_to_analog_rising_trigger_mode()

        self._x_controller.set_analog_rising_trigger_params(
            ANALOG_IN_GAIN_0_TO_10,
            ANALOG_IN_OFFSET_0_TO_10,
            ANALOG_OUT_GAIN_0_TO_10,
            ANALOG_OUT_OFFSET_0_TO_10,
        )

        self._y_controller.set_analog_rising_trigger_params(
            ANALOG_IN_GAIN_0_TO_10,
            ANALOG_IN_OFFSET_0_TO_10,
            ANALOG_OUT_GAIN_0_TO_10,
            ANALOG_OUT_OFFSET_0_TO_10,
        )

    def calibrate(self) -> None:
        """Run calibration by printing a continuous stream of intensities."""
        self._scope.set_mode(SCOPE_MODE_SINGLE)
        self._scope.set_sample_region(0, 4000)

        try:
            while True:
                self._scope.configure_single_capture()
                self._scope.run_capture()

                reading = self._scope.get_single()
                intensity_mv = np.mean(reading["intensity_mv"])

                _clear_terminal()
                print(
                    "Current Intensity (mV):\n"
                    "\n"
                    f"{intensity_mv}\n"
                    "\n"
                    "Ctrl+C to quit"
                )

                time.sleep(CALIBRATION_REFRESH_S)

        except KeyboardInterrupt:
            _clear_terminal()
            print("\nStopped voltage monitor.")

    def profile(self, port: str, instruction_path: str) -> None:
        """Run profiling with a set of instructions."""
        try:
            with open(instruction_path, encoding="utf-8") as f:
                instruction = json.load(f)

        except json.JSONDecodeError as e:
            raise InvalidInstructionJSONException(
                f"Invalid JSON in '{instruction_path}': {e}"
            ) from e

        check_no_none(instruction)

        self._scope.set_mode(SCOPE_MODE_BULK)
        self._scope.set_sample_region(
            instruction["capture"]["posttrigger_time_ns"],
            instruction["capture"]["posttrigger_time_ns"],
            instruction["capture"]["sample_interval_ns"],
        )

        try:
            ser = serial.Serial(port, BAUD_RATE, timeout=None)

            if MODE_MAP[instruction["mode"]] == MODE_CONTINUOUS:
                ser.write(mcu_instruction(instruction))

                while True:
                    self._scope.configure_bulk_capture(
                        instruction["capture"]["num_pulses"]
                    )
                    self._scope.run_capture()

                    line = ser.readline()
                    if not line:
                        continue

                    try:
                        status = json.loads(line)

                    except json.JSONDecodeError as e:
                        raise InvalidInstructionJSONException(
                            f"Invalid JSON in '{instruction_path}': {e}"
                        ) from e

                    if status["ok"] and status["msg"] == "done":
                        break

            else:
                num_points = (
                    instruction["grid"]["x"]["max"]
                    - instruction["grid"]["x"]["min"]
                    + 1
                ) * (
                    instruction["grid"]["y"]["max"]
                    - instruction["grid"]["y"]["min"]
                    + 1
                )

                ser.write(mcu_instruction(instruction))

                for _ in range(num_points):
                    self._scope.configure_bulk_capture(
                        instruction["capture"]["num_pulses"]
                    )
                    self._scope.run_capture()

        except KeyError as e:
            raise InvalidModeException("Mode provided is not valid.") from e
