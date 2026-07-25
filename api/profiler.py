"""This module contains the main profiling functionality."""

import json
import time
from typing import Final

import numpy as np
import serial
from serial.tools import list_ports

from api.params import (
    ContinuousCaptureParams,
    PointCountCaptureParams,
    PointTimeCaptureParams,
    ProfilerParams,
)
from api.utils import ceil_div
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

_MODE_STR_FROM_CAPTURE_MODE_TYPE: Final[dict] = {
    PointCountCaptureParams: "point_count",
    PointTimeCaptureParams: "point_time",
    ContinuousCaptureParams: "continuous",
}

X_PDXC2_SERIAL_NUM: Final[bytes] = b"112547939"
Y_PDXC2_SERIAL_NUM: Final[bytes] = b"112512664"

CALIBRATION_REFRESH_S: Final[float] = 0.100

PORT_WAIT_TIMEOUT_S: Final[float] = 10.0
PORT_POLL_INTERVAL_S: Final[float] = 0.01
BAUD_RATE: Final[int] = 115200


class InvalidMode(Exception):
    """Exception for when the provided mode is not valid."""


class PortWaitTimeout(Exception):
    """When the host times out waiting for the device on the port."""


class InvalidJSONFromMCU(Exception):
    """When an invalid JSON is received from the microcontroller."""


class MCUTimeout(Exception):
    """Host timed out waiting for a message from the microcontroller."""


class MCUError(Exception):
    """Host receives an error message from the microcontroller."""


def _clear_terminal() -> None:
    """Clear the terminal."""
    print("\033[2J\033[H", end="")


def _ser_write_newline_terminated(ser: serial.Serial, params: dict):
    """Write a newline-terminated JSON over serial."""
    ser.write((json.dumps(params) + "\n").encode())


class Profiler:
    """Profiler class responsible for controlling scope and controllers."""

    def __enter__(self) -> "Profiler":
        """Return the instance."""
        return self

    def __exit__(self, *exc) -> None:
        """Cleanup enabled controllers and open scope."""
        self._x_controller.close()
        self._y_controller.close()
        self._scope.close()

    def __init__(self) -> None:
        """Initialize a profiler instance."""
        self._scope = Scope()

        self._x_controller = Controller(X_PDXC2_SERIAL_NUM)
        self._y_controller = Controller(Y_PDXC2_SERIAL_NUM)

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

        self._scope.disable_trigger()

    def calibrate(self) -> None:
        """Run calibration by printing a continuous stream of intensities."""
        self._scope.set_mode(SCOPE_MODE_SINGLE)
        self._scope.set_sample_region(0, 4000)

        try:
            self._scope.disable_trigger_a()

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

    def profile(self, port: str, params: ProfilerParams) -> None:
        """Run profiling with a set of instructions."""
        self._scope.set_sample_region(
            params.capture.pretrigger_time_ns,
            params.capture.posttrigger_time_ns,
            params.capture.sample_interval_ns,
        )
        self._scope.set_mode(SCOPE_MODE_BULK)

        start = time.time()

        while True:
            if time.time() - start >= PORT_WAIT_TIMEOUT_S:
                raise PortWaitTimeout("Timed out waiting for port.")

            available_ports = [p.device for p in list_ports.comports()]

            if port in available_ports:
                break

            time.sleep(PORT_POLL_INTERVAL_S)

        self._ser = serial.Serial(port, BAUD_RATE, timeout=None)

        match params.capture:
            case PointCountCaptureParams():
                self._profile_mode_point_count()

            case PointTimeCaptureParams():
                self._profile_mode_point_count()

            case ContinuousCaptureParams():
                self._profile_mode_continuous()

        self._ser.close()

    def _poll_mcu_status(self, timeout_s: float = 10.0) -> dict:
        """Read one status line from the MCU, raising on any error status."""
        start = time.time()

        while time.time() - start < timeout_s:
            line = self._ser.readline()
            if not line:
                continue

            try:
                status = json.loads(line)
                return status

            except json.JSONDecodeError as e:
                raise InvalidJSONFromMCU(
                    f"Invalid JSON from microcontroller: {e}"
                ) from e

    def _profile_mode_point_count(self, params: ProfilerParams) -> None:
        """Profile a beam in `point_count` mode."""
        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "point_count",
                "x_min": params.grid.x.min,
                "x_max": params.grid.x.max,
                "x_unit_nm": params.grid.x.unit_nm,
                "x_origin_nm": params.grid.x.origin_nm,
                "y_min": params.grid.y.min,
                "y_max": params.grid.y.max,
                "y_unit_nm": params.grid.y.unit_nm,
                "y_origin_nm": params.grid.y.origin_nm,
                "num_pulses": params.capture.num_pulses,
                "posttrigger_time_us": ceil_div(
                    params.capture.posttrigger_time_ns, 1000
                ),
            },
        )

        if self._poll_mcu_status()["ok"]:
            self._scope.enable_trigger_a()

            for _ in range(params.grid.num_points):
                self._scope.configure_bulk_capture(
                    params.capture.num_pulses
                )
                self._scope.run_capture()

            self._scope.disable_trigger()

    def _profile_mode_point_time(self, params: ProfilerParams) -> None:
        """Profile a beam in `point_time` mode."""
        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "point_time",
                "x_min": params.grid.x.min,
                "x_max": params.grid.x.max,
                "x_unit_nm": params.grid.x.unit_nm,
                "x_origin_nm": params.grid.x.origin_nm,
                "y_min": params.grid.y.min,
                "y_max": params.grid.y.max,
                "y_unit_nm": params.grid.y.unit_nm,
                "y_origin_nm": params.grid.y.origin_nm,
                "wait_time_us": params.capture.wait_time_us,
                "posttrigger_time_us": ceil_div(
                    params.capture.posttrigger_time_ns, 1000
                ),
            },
        )

        if self._poll_mcu_status()["ok"]:
            self._scope.enable_trigger_a()

            for _ in range(params.grid.num_points):
                self._scope.configure_single_capture()
                self._scope.run_capture()

            self._scope.disable_trigger()

    def _profile_mode_continuous(self, params: ProfilerParams) -> None:
        """Profile a beam in `continuous` mode."""
        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "continuous",
                "x_min": params.grid.x.min,
                "x_max": params.grid.x.max,
                "x_unit_nm": params.grid.x.unit_nm,
                "x_origin_nm": params.grid.x.origin_nm,
                "y_min": params.grid.y.min,
                "y_max": params.grid.y.max,
                "y_unit_nm": params.grid.y.unit_nm,
                "y_origin_nm": params.grid.y.origin_nm,
            },
        )

        if self._poll_mcu_status()["ok"]:
            self._scope.enable_trigger_a()

            while True:
                self._scope.configure_single_capture()
                self._scope.run_capture()

                status = self._poll_mcu_status(timeout_s=0.010)

                if not status["ok"]:
                    error = status["msg"]
                    raise MCUError(f"Microcontroller error message: {error}")

                elif status["ok"] and status["msg"] == "profile_done":
                    self._scope.disable_trigger()
                    break
