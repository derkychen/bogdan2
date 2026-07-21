"""This module contains the main profiling functionality."""

import json
import time
from typing import Final

import numpy as np
import serial
from serial.tools import list_ports

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
    check_no_none,
    mcu_instruction,
)

X_PDXC2_SERIAL_NUM: Final[bytes] = b"112547939"
Y_PDXC2_SERIAL_NUM: Final[bytes] = b"112512664"

CALIBRATION_REFRESH_S: Final[float] = 0.100

PORT_WAIT_TIMEOUT_S: Final[float] = 10.0
PORT_POLL_INTERVAL_S: Final[float] = 0.01
BAUD_RATE: Final[int] = 115200


class InvalidInstructionJSON(Exception):
    """When an invalid JSON is in the instruction file."""


class InvalidMode(Exception):
    """Exception for when the provided mode is not valid."""


class PortWaitTimeout(Exception):
    """When the host times out waiting for the device on the port."""


class InvalidJSONFromMCU(Exception):
    """When an invalid JSON is received from the microcontroller."""


def _clear_terminal() -> None:
    """Clear the terminal."""
    print("\033[2J\033[H", end="")


class Profiler:
    """Profiler class responsible for controlling scope and controllers."""

    def __init__(self) -> None:
        """Initialize a profiler instance."""
        self._scope = Scope()

        self._x_controller = Controller(X_PDXC2_SERIAL_NUM)
        self._y_controller = Controller(Y_PDXC2_SERIAL_NUM)

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
            raise InvalidInstructionJSON(
                f"Invalid JSON in '{instruction_path}': {e}"
            ) from e

        check_no_none(instruction)

        self._scope.set_sample_region(
            instruction["capture"]["pretrigger_time_ns"],
            instruction["capture"]["posttrigger_time_ns"],
            instruction["capture"]["sample_interval_ns"],
        )
        self._scope.set_mode(SCOPE_MODE_BULK)

        start = time.time()
        while True:
            if time.time() - start < PORT_WAIT_TIMEOUT_S:
                raise PortWaitTimeout("Timed out waiting for port.")

            available_ports = [p.device for p in list_ports.comports()]

            if port in available_ports:
                break

            time.sleep(PORT_POLL_INTERVAL_S)

        with serial.Serial(port, BAUD_RATE, timeout=None) as ser:
            mode = instruction["mode"]

            if mode == "continuous":
                ser.write(json.dumps(mcu_instruction(instruction)).encode())

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
                        raise InvalidInstructionJSON(
                            f"Invalid JSON in '{instruction_path}': {e}"
                        ) from e

                    if status["ok"] and status["msg"] == "done":
                        break

                    # TODO: Process data.

            elif mode == "point_count" or mode == "point_time":
                num_points = (
                    instruction["grid"]["x"]["max"]
                    - instruction["grid"]["x"]["min"]
                    + 1
                ) * (
                    instruction["grid"]["y"]["max"]
                    - instruction["grid"]["y"]["min"]
                    + 1
                )

                ser.write(json.dumps(mcu_instruction(instruction)).encode())

                for _ in range(num_points):
                    self._scope.configure_bulk_capture(
                        instruction["capture"]["num_pulses"]
                    )
                    self._scope.run_capture()

                # TODO: Process data.

            else:
                raise InvalidMode(f"Invalid mode '{mode}'.")
