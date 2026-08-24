"""Main profiling functionality."""

import json
import time
from contextlib import ExitStack
from dataclasses import dataclass
from typing import Final, Self, cast, override

import numpy as np
import numpy.typing as npt
import serial
from serial.tools import list_ports

from bogdan2._pdxc2 import Controller, ControlModeID, TriggerModeID
from bogdan2._pico import (
    ChannelParams,
    RangeID,
    Scope,
    TriggerDirectionID,
)
from bogdan2._utils import ceil_div
from bogdan2.api.data import (
    BeamPoint,
    BeamProfile,
)

# TODO: Change to -10 to 10 volts when Thorlabs fixes the gain and offset bug.
MIN_MV: Final[float] = 0.0
MAX_MV: Final[float] = 10_000.0

ANALOG_IN_GAIN: Final[float] = 1.0
ANALOG_IN_OFFSET_MV: Final[float] = -10_000.0
ANALOG_OUT_GAIN: Final[float] = 1.0
ANALOG_OUT_OFFSET_MV: Final[float] = 0.0

PORT_WAIT_TIMEOUT_S: Final[float] = 10.0
PORT_POLL_INTERVAL_S: Final[float] = 0.01
BAUD_RATE: Final[int] = 115_200

AXIS_MIN_MIN: Final[int] = -1000
AXIS_MAX_MAX: Final[int] = 1000
AXIS_UNIT_MIN_MM: Final[float] = 0.002_000
AXIS_UNIT_MAX_MM: Final[float] = 1.000_000

# TODO: Extend stage range to +/- 6 mm if Thorlabs fixes the gain and offset
#       bug.
AXIS_STAGE_RANGE_MIN_MM: Final[float] = -3.000_000
AXIS_STAGE_RANGE_MAX_MM: Final[float] = 3.000_000

CAPTURE_SAMPLE_INTERVAL_NS_MAX: Final[int] = 10_000
CAPTURE_MAX_SAMPLES: Final[int] = 30_000_000

POINTCOUNT_CAPTURE_NUM_PULSES_MAX: Final[int] = 100
POINTTIME_CAPTURE_WAIT_TIME_MAX_US: Final[int] = 1_000_000_000


class MCUError(Exception):
    """When the host receives an error message from the microcontroller."""


class MCUProtocolError(Exception):
    """When the microcontroller returned an unexpected status."""


@dataclass(kw_only=True)
class _BaseCaptureParams:
    """Define a parent configuration for capture."""

    pretrigger_time_ns: int
    posttrigger_time_ns: int
    sample_interval_ns: int

    def __post_init__(self) -> None:
        """Validate base capture parameters."""
        if self.pretrigger_time_ns < 0:
            raise ValueError("Pre-trigger time must not be negative.")

        if self.posttrigger_time_ns < 0:
            raise ValueError("Post-trigger time must not be negative.")

        if self.pretrigger_time_ns + self.posttrigger_time_ns <= 0:
            raise ValueError("Capture duration must be greater than 0.")

        if self.sample_interval_ns <= 0:
            raise ValueError("Sample interval must be greater than 0.")

        if self.sample_interval_ns > CAPTURE_SAMPLE_INTERVAL_NS_MAX:
            raise ValueError(
                "Sample interval length exceeds maximum sample interval "
                + f"({CAPTURE_SAMPLE_INTERVAL_NS_MAX})."
            )

        max_samples = self.num_captures * ceil_div(
            self.pretrigger_time_ns + self.posttrigger_time_ns,
            self.sample_interval_ns,
        )

        if max_samples > CAPTURE_MAX_SAMPLES:
            raise ValueError(
                f"Maximum number of samples ({max_samples}) exceeds the "
                + f"limit {CAPTURE_MAX_SAMPLES}."
            )

    @property
    def num_captures(self) -> int:
        raise NotImplementedError


def _mm_to_nm(mm: float) -> int:
    """Convert a quantity in millimetres to nanometres."""
    return int(mm * 1e6)


def _mv_capture_to_mm(
    mv: npt.NDArray[np.float64] | list[npt.NDArray[np.float64]],
) -> float:
    """Convert x and y millivolt readings into a position."""
    return (
        (float(np.mean(mv)) - MIN_MV)
        * (AXIS_STAGE_RANGE_MAX_MM - AXIS_STAGE_RANGE_MIN_MM)
        / (MAX_MV - MIN_MV)
    )


def _parse_mcu_msg(line: bytes) -> str:
    """Parse a microcontroller status line and return its message."""
    try:
        status = cast(dict[str, bool | str], json.loads(line))
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON from microcontroller: {e}") from e

    print(f"Read from microcontroller: {status}")

    if not status["ok"]:
        raise MCUError(f"Microcontroller error: {status['msg']}")

    return cast(str, status["msg"])


@dataclass(kw_only=True)
class AxisParams:
    """Define one axis.

    Attributes
    ----------
    min : int
        The minimum/smallest coordinate on the axis.
    max : int
        The maximum/largest coordinate on the axis.
    unit_mm : float
        The unit length in millimetres (the separation between each point on
        the axis).
    origin_mm : float
        The location of the axis origin relative to the stage origin in
        millimetres.
    """

    min: int
    max: int
    unit_mm: float
    origin_mm: float

    def __post_init__(self) -> None:
        """Validate axis parameters.

        Raises
        ------
        ValueError
            If the supplied parameters result in an axis incompatible with
            either software or hardware.
        """
        if self.min < AXIS_MIN_MIN:
            raise ValueError(
                f"Minimum coordinate is too small (<{AXIS_MIN_MIN})."
            )

        if self.max > AXIS_MAX_MAX:
            raise ValueError(
                f"Maximum coordinate is too large (>{AXIS_MAX_MAX})."
            )

        if self.min > self.max:
            raise ValueError(
                "Minimum coordinate is greater than maximum coordinate."
            )

        if self.unit_mm < AXIS_UNIT_MIN_MM:
            raise ValueError(
                "The unit length is less than the minimum step length of the "
                + f"stage ({AXIS_UNIT_MIN_MM} mm)."
            )

        if self.unit_mm > AXIS_UNIT_MAX_MM:
            raise ValueError(
                f"The unit length is too large (>{AXIS_UNIT_MAX_MM} mm)."
            )

        if (
            self.origin_mm < AXIS_STAGE_RANGE_MIN_MM
            or self.origin_mm > AXIS_STAGE_RANGE_MAX_MM
        ):
            raise ValueError(
                f"Origin {self.origin_mm} is not within the stage range "
                + f"({AXIS_STAGE_RANGE_MIN_MM} to {AXIS_STAGE_RANGE_MAX_MM} "
                + "mm)."
            )

        min_mm = self.origin_mm + (self.min * self.unit_mm)
        max_mm = self.origin_mm + (self.max * self.unit_mm)

        if (
            min_mm < AXIS_STAGE_RANGE_MIN_MM
            or max_mm > AXIS_STAGE_RANGE_MAX_MM
        ):
            raise ValueError(
                "The bounds of the axis are not within the stage range "
                + f"({AXIS_STAGE_RANGE_MIN_MM} to {AXIS_STAGE_RANGE_MAX_MM} "
                + "mm)."
            )

    @property
    def num_points(self) -> int:
        """The number of points on the axis.

        Returns
        -------
        int
            The number of points on the axis.
        """
        return self.max - self.min + 1


@dataclass(kw_only=True)
class GridParams:
    """Define a grid with an x and y axis.

    Attributes
    ----------
    x : AxisParams
        Parameters for the x-axis.
    y : AxisParams
        Parameters for the y-axis.
    """

    x: AxisParams
    y: AxisParams

    @property
    def num_points(self) -> int:
        """Number of points on the grid.

        Returns
        -------
        int
            The number of points on the grid.
        """
        return self.x.num_points * self.y.num_points


@dataclass(kw_only=True)
class PointCountCaptureParams(_BaseCaptureParams):
    """Define a capture in Point-Count mode.

    Attributes
    ----------
    pretrigger_time_ns : int
        The desired range of time to capture before the trigger is received.
    posttrigger_time_ns : int
        The desired range of time to capture after the trigger is received.
    sample_interval_ns : int
        The desired number of nanoseconds to elapse per oscilloscope sample.
    num_pulses : int
        The number of pulses to capture at each point on the grid.
    """

    num_pulses: int

    def __post_init__(self) -> None:
        """Validate Point-Count capture parameters.

        Raises
        ------
        ValueError
            If the number of pulses is invalid, since the parent checks the
            validity of the sample region.
        """
        super().__post_init__()

        if self.num_pulses <= 0:
            raise ValueError(
                "Number of pulses per point for Point-Count capture must be "
                + "greater than 0."
            )

        if self.num_pulses > POINTCOUNT_CAPTURE_NUM_PULSES_MAX:
            raise ValueError(
                "Number of pulses per point for Point-Count capture "
                + f"({self.num_pulses}) exceeds the maximum number of pulses "
                + f"per point ({POINTCOUNT_CAPTURE_NUM_PULSES_MAX})."
            )

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each pulse.

        Returns
        -------
        int
            The number of pulses, since one capture is done for each pulse.
        """
        return self.num_pulses


@dataclass(kw_only=True)
class PointTimeCaptureParams(_BaseCaptureParams):
    """Define a capture in Point-Time mode.

    Attributes
    ----------
    pretrigger_time_ns : int
        The desired range of time to capture before the trigger is received.
    posttrigger_time_ns : int
        The desired range of time to capture after the trigger is received.
    sample_interval_ns : int
        The desired number of nanoseconds to elapse per oscilloscope sample.
    wait_time_us : int
        The amount of time the stages wait at each point. This can differ from
        the actual amount of time that the oscilloscope captures for.
    """

    wait_time_us: int

    def __post_init__(self) -> None:
        """Validate Point-Time capture parameters.

        Raises
        ------
        ValueError
            If the number of pulses is invalid, since the parent checks the
            validity of the sample region.
        """
        super().__post_init__()

        if self.wait_time_us <= 0:
            raise ValueError(
                "Number of microseconds to wait per point for Point-Time "
                + "capture must be greater than 0."
            )

        if self.wait_time_us > POINTTIME_CAPTURE_WAIT_TIME_MAX_US:
            raise ValueError(
                "Number of microseconds to wait per point for Point-Time "
                + f"capture ({self.wait_time_us}) exceeds the maximum wait "
                + f"time per point ({POINTTIME_CAPTURE_WAIT_TIME_MAX_US} us)."
            )

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each time window.

        Returns
        -------
        int
            Always 1 since each capture is a single acquisition over a
            specified duration.
        """
        return 1


@dataclass(kw_only=True)
class ContinuousCaptureParams(_BaseCaptureParams):
    """Define a capture in Continuous mode.

    Attributes
    ----------
    pretrigger_time_ns : int
        The desired range of time to capture before the trigger is received.
    posttrigger_time_ns : int
        The desired range of time to capture after the trigger is received.
    sample_interval_ns : int
        The desired number of nanoseconds to elapse per oscilloscope sample.
    """

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each time window.

        Returns
        -------
        int
            Always 1 since each capture since each capture no waiting occurs,
            captures come as single arbitrary points on the path traversed by
            the stages.
        """
        return 1


type CaptureParams = (
    PointCountCaptureParams | PointTimeCaptureParams | ContinuousCaptureParams
)


@dataclass(kw_only=True)
class BeamProfilerParams:
    """Define a full set of profiling parameters.

    Attributes
    ----------
    grid : GridParams
        The grid within which points should be captured.
    capture : CaptureParams
        The capture parameters including samples around the trigger as well as
        parameters specific to a capture approach.
    """

    grid: GridParams
    capture: CaptureParams


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamProfilerID:
    """An identifier for a beam profiler.

    Attributes
    ----------
    port : str
        The serial port on which the microcontroller is listening.
    picoscope_serial_num : str
        The serial number of the PicoScope, which is located on a label on its
        back.
    x_pdxc2_serial_num : str
        The serial number of the x-axis PDXC2, prefixed with "SN:". This prefix
        should be omitted in instantiation. It is located on the side where
        power is supplied.
    y_pdxc2_serial_num : str
        The serial number of the y-axis PDXC2, prefixed with "SN:". This prefix
        should be omitted in instantiation. It is located on the side where
        power is supplied.
    """

    port: str
    picoscope_serial_num: str
    x_pdxc2_serial_num: str
    y_pdxc2_serial_num: str


class BeamProfiler:
    """Profiler responsible for controlling oscilloscope and controllers."""

    def __init__(self, id: BeamProfilerID) -> None:
        """Initialize the profiler. Acquires no hardware.

        Parameters
        ----------
        id : BeamProfilerID
            The `BeamProfilerID` object that contains strings to uniquely
            identify one beam profiler.
        """
        self._stack: ExitStack[bool | None]

        self._port: str = id.port
        self._scope: Scope = Scope(id.picoscope_serial_num)
        self._x_controller: Controller = Controller(id.x_pdxc2_serial_num)
        self._y_controller: Controller = Controller(id.y_pdxc2_serial_num)

        self._ser: serial.Serial | None = None
        self._ser_rx_buf: bytearray = bytearray()

    def __enter__(self) -> Self:
        """Acquire hardware.

        Returns
        -------
        Self
            The `BeamProfiler` instance.
        """
        with ExitStack() as stack:
            self._scope.open()
            _ = stack.callback(self._scope.close)

            self._x_controller.open()
            _ = stack.callback(self._x_controller.close)

            self._y_controller.open()
            _ = stack.callback(self._y_controller.close)

            self._configure()

            self._stack = stack.pop_all()

        return self

    def __exit__(self, *exc: object) -> None:
        """Cleanup hardware.

        Parameters
        ----------
        *exc : tuple of object
            Ignored exception arguments.
        """
        if self._ser is not None:
            self._ser.close()
        self._stack.close()

    def profile(self, params: BeamProfilerParams) -> BeamProfile:
        """Run profiling with a set of parameters.

        Parameters
        ----------
        params : ProfilerParams
            Parameters for the beam profile which define a grid and a
            capture approach

        Returns
        -------
        BeamProfile
            An object that stores the raw data for analysis and
            visualization.

        Raises
        ------
        TimeoutError
            If connection to the port on which the microcontroller is takes too
            long, or if the host times out waiting for a response from the
            microcontroller.
        ValueError
            If an invalid JSON packet is received from the microcontroller.
        MCUError
            If the host receives an error message from the microcontroller.
        MCUProtocolError
            If the host receives an unexpected message from the
            microcontroller.
        """
        deadline = time.monotonic() + PORT_WAIT_TIMEOUT_S

        while True:
            if time.monotonic() >= deadline:
                raise TimeoutError("Timed out waiting for port.")

            available_ports = [p.device for p in list_ports.comports()]

            if self._port in available_ports:
                break

            time.sleep(PORT_POLL_INTERVAL_S)

        self._ser = serial.Serial(self._port, BAUD_RATE, timeout=0.0)
        self._ser_rx_buf.clear()

        try:
            match params.capture:
                case PointCountCaptureParams() as capture:
                    return self._profile_mode_point_count(params.grid, capture)

                case PointTimeCaptureParams() as capture:
                    return self._profile_mode_point_time(params.grid, capture)

                case ContinuousCaptureParams() as capture:
                    return self._profile_mode_continuous(params.grid, capture)

        finally:
            self._ser.close()
            self._ser = None

    def _configure(self) -> None:
        """Configure the profiler hardware."""
        self._x_controller.ensure_trigger_mode(TriggerModeID.MANUAL)
        self._y_controller.ensure_trigger_mode(TriggerModeID.MANUAL)

        # Use fast settings.
        #
        # TODO: Set acceleration as 1000 mm/s^2 after confirming it is okay for
        #       long-term usage.
        self._x_controller.ensure_closedloop_params(
            refspeed=15_000_000, acceleration=500_000_000
        )
        self._y_controller.ensure_closedloop_params(
            refspeed=15_000_000, acceleration=500_000_000
        )

        self._x_controller.ensure_control_mode(ControlModeID.CLOSED_LOOP)
        self._y_controller.ensure_control_mode(ControlModeID.CLOSED_LOOP)

        self._x_controller.ensure_analog_rising_trigger_params(
            in_gain=ANALOG_IN_GAIN,
            in_offset=ANALOG_IN_OFFSET_MV,
            out_gain=ANALOG_OUT_GAIN,
            out_offset=ANALOG_OUT_OFFSET_MV,
        )
        self._y_controller.ensure_analog_rising_trigger_params(
            in_gain=ANALOG_IN_GAIN,
            in_offset=ANALOG_IN_OFFSET_MV,
            out_gain=ANALOG_OUT_GAIN,
            out_offset=ANALOG_OUT_OFFSET_MV,
        )

        self._x_controller.ensure_trigger_mode(TriggerModeID.ANALOG_RISING)
        self._y_controller.ensure_trigger_mode(TriggerModeID.ANALOG_RISING)

        self._scope.setup()
        self._scope.configure_channels(
            a_params=ChannelParams(name="trigger_mv", range_id=RangeID.V20),
            b_params=ChannelParams(name="x_mv", range_id=RangeID.V20),
            c_params=ChannelParams(name="y_mv", range_id=RangeID.V20),
            d_params=ChannelParams(name="intensity_mv", range_id=RangeID.V20),
        )
        self._scope.disable_trigger_a()

    def _ser_write_newline_terminated(
        self, data: dict[str, bool | int | str]
    ) -> None:
        """Write a newline-terminated JSON over serial."""
        assert self._ser is not None, "Serial connection must be initialized."

        raw = json.dumps(data, separators=(",", ":"))

        print(f"Wrote to microcontroller: {raw}")

        _ = self._ser.write((raw + "\n").encode())

    def _check_mcu_status(self) -> str | None:
        """Check for status from the microcontroller if it is available."""
        assert self._ser is not None, "Serial connection must be initialized."

        num_waiting = self._ser.in_waiting

        if num_waiting:
            self._ser_rx_buf.extend(self._ser.read(num_waiting))

        newline = self._ser_rx_buf.find(b"\n")

        if newline < 0:
            return None

        line = bytes(self._ser_rx_buf[:newline])
        del self._ser_rx_buf[: newline + 1]

        return _parse_mcu_msg(line)

    def _poll_mcu_status(self, timeout_s: float = 10.0) -> str:
        """Poll for status from the microcontroller."""
        deadline = time.monotonic() + timeout_s

        while time.monotonic() < deadline:
            status = self._check_mcu_status()

            if status is not None:
                return status

            time.sleep(0.001)

        raise TimeoutError("Timed out waiting for microcontroller status.")

    def _expect_mcu_status(
        self,
        expected: str,
        timeout_s: float = 10.0,
    ) -> None:
        """Wait for a specific microcontroller status."""
        actual = self._poll_mcu_status(timeout_s)

        if actual != expected:
            raise MCUProtocolError(
                f"Expected status {expected!r}, got {actual!r}."
            )

    def _beam_point_single(self) -> BeamPoint:
        """Construct a beam point from a single capture."""
        interval_s = self._scope.get_sample_interval_ns() * 1e-9

        x_mm = _mv_capture_to_mm(self._scope.channel_single_mv("x_mv"))
        y_mm = _mv_capture_to_mm(self._scope.channel_single_mv("y_mv"))

        intensity_mv = self._scope.channel_single_mv("intensity_mv")

        print(intensity_mv)

        intensity = float(np.trapezoid(intensity_mv, dx=interval_s))

        return BeamPoint(x_mm=x_mm, y_mm=y_mm, intensity=intensity)

    def _beam_point_bulk(self) -> BeamPoint:
        """Construct a beam point from a single capture."""
        interval_s = self._scope.get_sample_interval_ns() * 1e-9

        x_mm = _mv_capture_to_mm(self._scope.channel_bulk_mv("x_mv"))
        y_mm = _mv_capture_to_mm(self._scope.channel_bulk_mv("y_mv"))

        intensity_captures = self._scope.channel_bulk_mv("intensity_mv")
        intensity = float(
            np.mean(
                [
                    np.trapezoid(capture, dx=interval_s)
                    for capture in intensity_captures
                ]
            )
        )

        return BeamPoint(x_mm=x_mm, y_mm=y_mm, intensity=intensity)

    def _profile_mode_point_count(
        self, grid: GridParams, capture: PointCountCaptureParams
    ) -> BeamProfile:
        """Profile a beam in Point-Count mode."""
        self._scope.set_sample_region(
            pretrigger_time_ns=capture.pretrigger_time_ns,
            posttrigger_time_ns=capture.posttrigger_time_ns,
            sample_interval_ns=capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        self._ser_write_newline_terminated(
            {
                "mode": "point_count",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": _mm_to_nm(grid.x.unit_mm),
                "x_origin_nm": _mm_to_nm(grid.x.origin_mm),
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": _mm_to_nm(grid.y.unit_mm),
                "y_origin_nm": _mm_to_nm(grid.y.origin_mm),
                "num_pulses": capture.num_pulses,
                "posttrigger_time_us": ceil_div(
                    self._scope.get_posttrigger_ns(), 1000
                ),
            }
        )

        _ = self._poll_mcu_status()

        points: list[BeamPoint] = []

        self._scope.enable_trigger_a(TriggerDirectionID.RISING)
        self._scope.configure_bulk_capture(capture.num_pulses)

        try:
            for i in range(1, grid.num_points + 1):
                print(f"Point #{i}")

                self._scope.arm_capture()
                self._ser_write_newline_terminated({"cmd": "go_to_point"})

                try:
                    self._scope.poll_capture()
                except Exception:
                    _ = self._poll_mcu_status()
                    raise

                self._scope.transfer_bulk_values()
                points.append(self._beam_point_bulk())

        finally:
            self._scope.disable_trigger_a()

        profile = BeamProfile(points)

        try:
            self._expect_mcu_status("profile_done")
        except (MCUError, MCUProtocolError, TimeoutError) as e:
            print(f"Final movement was not confirmed: {e}")

        return profile

    def _profile_mode_point_time(
        self, grid: GridParams, capture: PointTimeCaptureParams
    ) -> BeamProfile:
        """Profile a beam in Point-Time mode."""
        self._scope.set_sample_region(
            pretrigger_time_ns=capture.pretrigger_time_ns,
            posttrigger_time_ns=capture.posttrigger_time_ns,
            sample_interval_ns=capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        self._ser_write_newline_terminated(
            {
                "mode": "point_time",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": _mm_to_nm(grid.x.unit_mm),
                "x_origin_nm": _mm_to_nm(grid.x.origin_mm),
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": _mm_to_nm(grid.y.unit_mm),
                "y_origin_nm": _mm_to_nm(grid.y.origin_mm),
                "wait_time_us": capture.wait_time_us,
            },
        )

        _ = self._poll_mcu_status()

        points: list[BeamPoint] = []

        self._scope.enable_trigger_a(TriggerDirectionID.RISING)
        self._scope.configure_single_capture()

        try:
            for i in range(1, grid.num_points + 1):
                print(f"Point #{i}")

                self._scope.arm_capture()
                self._ser_write_newline_terminated({"cmd": "go_to_point"})

                try:
                    self._scope.poll_capture()
                except Exception:
                    _ = self._poll_mcu_status()
                    raise

                self._scope.transfer_bulk_values()
                points.append(self._beam_point_single())

        finally:
            self._scope.disable_trigger_a()

        profile = BeamProfile(points)

        try:
            self._expect_mcu_status("profile_done")
        except (MCUError, MCUProtocolError, TimeoutError) as e:
            print(f"Final movement was not confirmed: {e}")

        return profile

    def _profile_mode_continuous(
        self, grid: GridParams, capture: ContinuousCaptureParams
    ) -> BeamProfile:
        """Profile a beam in Continuous mode."""
        self._scope.set_sample_region(
            pretrigger_time_ns=capture.pretrigger_time_ns,
            posttrigger_time_ns=capture.posttrigger_time_ns,
            sample_interval_ns=capture.sample_interval_ns,
        )

        assert self._ser is not None, "Serial connection must be initialized."

        self._ser_write_newline_terminated(
            {
                "mode": "continuous",
                "x_min": grid.x.min,
                "x_max": grid.x.max,
                "x_unit_nm": _mm_to_nm(grid.x.unit_mm),
                "x_origin_nm": _mm_to_nm(grid.x.origin_mm),
                "y_min": grid.y.min,
                "y_max": grid.y.max,
                "y_unit_nm": _mm_to_nm(grid.y.unit_mm),
                "y_origin_nm": _mm_to_nm(grid.y.origin_mm),
            },
        )

        _ = self._poll_mcu_status()

        points: list[BeamPoint] = []

        self._scope.enable_trigger_a(TriggerDirectionID.RISING)
        self._scope.configure_single_capture()

        i = 1

        try:
            while True:
                if self._check_mcu_status() == "profile_done":
                    break

                print(f"Point #{i}")

                self._scope.arm_capture()

                try:
                    self._scope.poll_capture(timeout_s=0.20)
                except TimeoutError:
                    if self._check_mcu_status() == "profile_done":
                        break
                    raise

                self._scope.transfer_bulk_values()
                points.append(self._beam_point_single())

                i += 1

        finally:
            self._scope.disable_trigger_a()

        return BeamProfile(points)
