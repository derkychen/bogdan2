"""Main profiling functionality."""

import json
import time
from contextlib import ExitStack
from typing import Final, Self, cast

import numpy as np
import serial
from serial.tools import list_ports

# from bogdan2._pdxc2.constants import (
#     ANALOG_IN_GAIN,
#     ANALOG_IN_OFFSET_MV,
#     ANALOG_OUT_GAIN,
#     ANALOG_OUT_OFFSET_MV,
# )
# from bogdan2._pdxc2.controller import Controller
from bogdan2._pico.constants import RANGE_20V
from bogdan2._pico.scope import Scope, ScopeChannelParams
from bogdan2._utils.math import ceil_div
from bogdan2.api.data import (
    BeamPoint,
    BeamProfile,
    Reading,
)
from bogdan2.api.params import (
    AXIS_STAGE_RANGE_MAX_NM,
    AXIS_STAGE_RANGE_MIN_NM,
    ContinuousCaptureParams,
    GridParams,
    PointCountCaptureParams,
    PointTimeCaptureParams,
    ProfilerParams,
)

ANALOG_OUT_MIN_MV: Final[float] = -10000.0
ANALOG_OUT_MAX_MV: Final[float] = 10000.0

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


def _ser_write_newline_terminated(
    ser: serial.Serial, params: dict[str, bool | int | str]
) -> None:
    """Write a newline-terminated JSON over serial."""
    _ = ser.write((json.dumps(params) + "\n").encode())


def _mv_to_mm(mv: Reading) -> float:
    """Convert x and y millivolt readings into a position."""
    return (
        (mv.mean - ANALOG_OUT_MIN_MV)
        * (AXIS_STAGE_RANGE_MAX_NM - AXIS_STAGE_RANGE_MIN_NM)
        / (ANALOG_OUT_MAX_MV - ANALOG_OUT_MIN_MV)
        * 1e-6
    )


class Profiler:
    """Profiler class responsible for controlling scope and controllers."""

    def __init__(self) -> None:
        """Initialize the profiler. Acquires no hardware."""
        self._stack: ExitStack[bool | None]

        self._scope: Scope = Scope()
        # self._x_controller: Controller = Controller(X_PDXC2_SERIAL_NUM)
        # self._y_controller: Controller = Controller(Y_PDXC2_SERIAL_NUM)
        self._ser: serial.Serial | None = None

    def __enter__(self) -> "Self":
        """Acquire hardware."""
        with ExitStack() as stack:
            # self._x_controller.open()
            # _ = stack.callback(self._x_controller.close)
            #
            # self._y_controller.open()
            # _ = stack.callback(self._y_controller.close)

            self._scope.open()
            _ = stack.callback(self._scope.close)

            self._configure()

            self._stack = stack.pop_all()

        return self

    def __exit__(self, *exc: object) -> None:
        """Cleanup hardware."""
        if self._ser is not None:
            self._ser.close()
        self._stack.close()

    def profile(self, port: str, params: ProfilerParams) -> BeamProfile | None:
        """Run profiling with a set of parameters."""
        start = time.time()

        while True:
            if time.time() - start >= PORT_WAIT_TIMEOUT_S:
                raise PortWaitTimeout("Timed out waiting for port.")

            available_ports = [p.device for p in list_ports.comports()]

            if port in available_ports:
                break

            time.sleep(PORT_POLL_INTERVAL_S)

        self._ser = serial.Serial(port, BAUD_RATE, timeout=1.0)

        profile = None

        match params.capture:
            case PointCountCaptureParams() as capture:
                profile = self._profile_mode_point_count(params.grid, capture)

            case PointTimeCaptureParams() as capture:
                profile = self._profile_mode_point_time(params.grid, capture)

            case ContinuousCaptureParams() as capture:
                profile = self._profile_mode_continuous(params.grid, capture)

            case _:
                raise InvalidMode(
                    f"Unsupported mode: {type(params.capture).__name__}"
                )

        self._ser.close()

        return profile

    def _configure(self) -> None:
        """Configure the profiler hardware."""
        # Use fast settings.
        #
        # TODO: Set acceleration as 1000 mm/s^2 after confirming it is okay for
        #       long-term usage.
        # self._x_controller.set_closedloop_params(
        #     refspeed=15000000, acceleration=500000000
        # )
        # self._y_controller.set_closedloop_params(
        #     refspeed=15000000, acceleration=500000000
        # )
        #
        # self._x_controller.set_to_analog_rising_trigger_mode()
        # self._y_controller.set_to_analog_rising_trigger_mode()
        #
        # self._x_controller.set_analog_rising_trigger_params(
        #     ANALOG_IN_GAIN_0_TO_10,
        #     ANALOG_IN_OFFSET_MV_0_TO_10,
        #     ANALOG_OUT_GAIN_N10_TO_10,
        #     ANALOG_OUT_OFFSET_MV_N10_TO_10,
        # )
        # self._y_controller.set_analog_rising_trigger_params(
        #     ANALOG_IN_GAIN_0_TO_10,
        #     ANALOG_IN_OFFSET_MV_0_TO_10,
        #     ANALOG_OUT_GAIN_N10_TO_10,
        #     ANALOG_OUT_OFFSET_MV_N10_TO_10,
        # )
        #
        # self._x_controller.persist_settings()
        # self._y_controller.persist_settings()

        self._scope.setup()
        self._scope.configure_channels(
            ScopeChannelParams(name="trigger_mv", range_id=RANGE_20V),
            ScopeChannelParams(name="x_mv", range_id=RANGE_20V),
            ScopeChannelParams(name="y_mv", range_id=RANGE_20V),
            ScopeChannelParams(name="intensity_mv", range_id=RANGE_20V),
        )
        self._scope.disable_trigger_a()

    def _poll_mcu_status(
        self, timeout_s: float = 10.0
    ) -> dict[str, bool | str] | None:
        """Read one status line from the MCU, raising on any error status."""
        start = time.time()

        assert self._ser is not None, "Serial connection must be initialized."

        while time.time() - start < timeout_s:
            line = self._ser.readline()
            if not line:
                continue

            try:
                status_dict = cast(dict[str, bool | str], json.loads(line))
                return status_dict

            except json.JSONDecodeError as e:
                raise InvalidJSONFromMCU(
                    f"Invalid JSON from microcontroller: {e}"
                ) from e

    def _beam_point_single(self) -> BeamPoint:
        """Construct a beam point from a single capture."""
        interval_s = self._scope.get_sample_interval_ns() * 1e-9

        x_mm = _mv_to_mm(Reading(vals=self._scope.channel_single_mv("x_mv")))
        y_mm = _mv_to_mm(Reading(vals=self._scope.channel_single_mv("y_mv")))

        intensity_mv = Reading(
            vals=self._scope.channel_single_mv("intensity_mv")
        )

        # TODO: Remove this debug statement.
        print(intensity_mv)

        intensity = intensity_mv.integral(interval_s)

        return BeamPoint(x_mm=x_mm, y_mm=y_mm, intensity=intensity)

    def _beam_point_bulk(self) -> BeamPoint:
        """Construct a beam point from a single capture."""
        interval_s = self._scope.get_sample_interval_ns() * 1e-9

        x_mm = _mv_to_mm(
            Reading(vals=np.concatenate(self._scope.channel_bulk_mv("x_mv")))
        )
        y_mm = _mv_to_mm(
            Reading(vals=np.concatenate(self._scope.channel_bulk_mv("y_mv")))
        )
        intensity = Reading(
            vals=np.concatenate(self._scope.channel_bulk_mv("intensity_mv"))
        ).integral(interval_s)

        return BeamPoint(x_mm=x_mm, y_mm=y_mm, intensity=intensity)

    def _profile_mode_point_count(
        self, grid: GridParams, capture: PointCountCaptureParams
    ) -> BeamProfile | None:
        """Profile a beam in `point_count` mode."""
        self._scope.set_sample_region(
            capture.pretrigger_time_ns,
            capture.posttrigger_time_ns,
            capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "point_count",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": grid.x.unit_nm,
                "x_origin_nm": grid.x.origin_nm,
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": grid.y.unit_nm,
                "y_origin_nm": grid.y.origin_nm,
                "num_pulses": capture.num_pulses,
                "posttrigger_time_us": ceil_div(
                    self._scope.get_posttrigger_ns(), 1000
                ),
            },
        )

        points: list[BeamPoint] = []

        status = self._poll_mcu_status()

        if status is not None:
            if status["ok"]:
                self._scope.enable_trigger_a()
                self._scope.configure_bulk_capture(capture.num_pulses)

                for _ in range(grid.num_points):
                    self._scope.run_capture()
                    self._scope.transfer_bulk_values()

                    points.append(self._beam_point_bulk())

                self._scope.disable_trigger_a()

                status = self._poll_mcu_status()

                if (
                    status is not None
                    and status["ok"]
                    and status["msg"] == "profile_done"
                ):
                    self._scope.disable_trigger_a()
            else:
                error = status["msg"]

                raise MCUError(f"Microcontroller error: {error}")

        return BeamProfile(points)

    def _profile_mode_point_time(
        self, grid: GridParams, capture: PointTimeCaptureParams
    ) -> BeamProfile | None:
        """Profile a beam in `point_time` mode."""
        self._scope.set_sample_region(
            capture.pretrigger_time_ns,
            capture.posttrigger_time_ns,
            capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "point_time",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": grid.x.unit_nm,
                "x_origin_nm": grid.x.origin_nm,
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": grid.y.unit_nm,
                "y_origin_nm": grid.y.origin_nm,
                "wait_time_us": capture.wait_time_us,
            },
        )

        points: list[BeamPoint] = []

        status = self._poll_mcu_status()

        if status is not None:
            if status["ok"]:
                self._scope.enable_trigger_a()
                self._scope.configure_single_capture()

                for _ in range(grid.num_points):
                    self._scope.run_capture()
                    self._scope.transfer_single_values()

                    points.append(self._beam_point_single())

                status = self._poll_mcu_status()

                if (
                    status is not None
                    and status["ok"]
                    and status["msg"] == "profile_done"
                ):
                    self._scope.disable_trigger_a()
            else:
                error = status["msg"]

                raise MCUError(f"Microcontroller error: {error}")

        return BeamProfile(points)

    def _profile_mode_continuous(
        self, grid: GridParams, capture: ContinuousCaptureParams
    ) -> BeamProfile | None:
        """Profile a beam in `continuous` mode."""
        self._scope.set_sample_region(
            capture.pretrigger_time_ns,
            capture.posttrigger_time_ns,
            capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        _ser_write_newline_terminated(
            self._ser,
            {
                "mode": "continuous",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": grid.x.unit_nm,
                "x_origin_nm": grid.x.origin_nm,
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": grid.y.unit_nm,
                "y_origin_nm": grid.y.origin_nm,
            },
        )

        points: list[BeamPoint] = []

        status = self._poll_mcu_status()

        if status is not None:
            if status["ok"]:
                self._scope.enable_trigger_a()
                self._scope.configure_single_capture()

                while True:
                    self._scope.run_capture()
                    self._scope.transfer_single_values()

                    points.append(self._beam_point_single())

                    status = self._poll_mcu_status(timeout_s=0.010)

                    if status is not None:
                        if not status["ok"]:
                            error = status["msg"]
                            raise MCUError(
                                f"Microcontroller error message: {error}"
                            )

                        elif status["ok"] and status["msg"] == "profile_done":
                            self._scope.disable_trigger_a()
                            break
            else:
                error = status["msg"]

                raise MCUError(f"Microcontroller error: {error}")

        return BeamProfile(points)
