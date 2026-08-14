"""Parameters used in describing a profile."""

from dataclasses import dataclass
from typing import Final, override

from bogdan2._utils.math import ceil_div

AXIS_MIN_MIN: Final[int] = -1_000
AXIS_MAX_MAX: Final[int] = 1_000
AXIS_UNIT_MAX_NM: Final[int] = 1_000_000

# TODO: Extend stage range to +/- 6 mm if Thorlabs fixes the gain and offset
#       bug.
AXIS_STAGE_RANGE_MIN_NM: Final[int] = -3_000_000
AXIS_STAGE_RANGE_MAX_NM: Final[int] = 3_000_000

AXIS_STAGE_MIN_STEP_NM: Final[int] = 300

CAPTURE_SAMPLE_INTERVAL_NS_MAX: Final[int] = 10_000
CAPTURE_MAX_SAMPLES: Final[int] = 30_000_000


class AxisParamsInitError(Exception):
    """When the axis parameters are incorrect."""


class GridParamsInitError(Exception):
    """When the grid parameters are incorrect."""


class CaptureParamsInitError(Exception):
    """When the capture parameters are incorrect."""


class ProfilerParamsInitError(Exception):
    """When the profiling parameters are incorrect."""


@dataclass(kw_only=True)
class AxisParams:
    """Define one axis."""

    min: int
    max: int
    unit_nm: int
    origin_nm: int

    def __post_init__(self) -> None:
        """Validate axis parameters."""
        if self.min < AXIS_MIN_MIN:
            raise AxisParamsInitError(
                f"Minimum coordinate is too small (<{AXIS_MIN_MIN})."
            )

        if self.max > AXIS_MAX_MAX:
            raise AxisParamsInitError(
                f"Maximum coordinate is too large (>{AXIS_MAX_MAX})."
            )

        if self.min > self.max:
            raise AxisParamsInitError(
                "Minimum coordinate is greater than maximum coordinate."
            )

        if self.unit_nm < AXIS_STAGE_MIN_STEP_NM:
            raise AxisParamsInitError(
                "The unit length is less than the minimum step length of the "
                + f"stage ({AXIS_STAGE_MIN_STEP_NM} nm)."
            )

        if self.unit_nm > AXIS_UNIT_MAX_NM:
            raise AxisParamsInitError(
                f"The unit length is too large (>{AXIS_UNIT_MAX_NM})."
            )

        if (
            self.origin_nm < AXIS_STAGE_RANGE_MIN_NM
            or self.origin_nm > AXIS_STAGE_RANGE_MAX_NM
        ):
            raise AxisParamsInitError(
                f"Origin {self.origin_nm} is not within the stage range "
                + f"({AXIS_STAGE_RANGE_MIN_NM} to {AXIS_STAGE_RANGE_MAX_NM} "
                + "nm)."
            )

        min_nm = self.origin_nm + (self.min * self.unit_nm)
        max_nm = self.origin_nm + (self.max * self.unit_nm)

        if (
            min_nm < AXIS_STAGE_RANGE_MIN_NM
            or max_nm > AXIS_STAGE_RANGE_MAX_NM
        ):
            raise AxisParamsInitError(
                "The bounds of the axis are not within the stage range "
                + f"({AXIS_STAGE_RANGE_MIN_NM} to {AXIS_STAGE_RANGE_MAX_NM} "
                + "nm)."
            )

    @property
    def num_points(self) -> int:
        """The number of points on the axis."""
        return self.max - self.min + 1


@dataclass(kw_only=True)
class GridParams:
    """Define a grid with an x and y axis."""

    x: AxisParams
    y: AxisParams

    @property
    def num_points(self) -> int:
        """Number of points on the grid."""
        return self.x.num_points * self.y.num_points


@dataclass(kw_only=True)
class _BaseCaptureParams:
    """Define a parent configuration for capture."""

    pretrigger_time_ns: int
    posttrigger_time_ns: int
    sample_interval_ns: int

    def __post_init__(self) -> None:
        """Validate base capture parameters."""
        if self.sample_interval_ns > CAPTURE_SAMPLE_INTERVAL_NS_MAX:
            raise CaptureParamsInitError(
                "Sample interval length exceeds maximum sample interval "
                + f"({CAPTURE_SAMPLE_INTERVAL_NS_MAX})."
            )

        max_samples = self.num_captures * ceil_div(
            self.pretrigger_time_ns + self.posttrigger_time_ns,
            self.sample_interval_ns,
        )

        if max_samples > CAPTURE_MAX_SAMPLES:
            raise CaptureParamsInitError(
                f"Maximum number of samples ({max_samples}) exceeds the "
                + f"limit {CAPTURE_MAX_SAMPLES}."
            )

    @property
    def num_captures(self) -> int:
        raise NotImplementedError


@dataclass(kw_only=True)
class PointCountCaptureParams(_BaseCaptureParams):
    """Define a capture in `point_count` mode."""

    num_pulses: int

    def __post_init__(self) -> None:
        """Validate `point_count` capture parameters."""
        super().__post_init__()

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each pulse."""
        return self.num_pulses


@dataclass(kw_only=True)
class PointTimeCaptureParams(_BaseCaptureParams):
    """Define a capture in `point_time` mode."""

    wait_time_us: int

    def __post_init__(self) -> None:
        """Validate `point_time` capture parameters."""
        super().__post_init__()

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each time window."""
        return 1


@dataclass(kw_only=True)
class ContinuousCaptureParams(_BaseCaptureParams):
    """Define a capture in `continuous` mode."""

    @property
    @override
    def num_captures(self) -> int:
        """Capture once for each time window."""
        return 1


@dataclass(kw_only=True)
class ProfilerParams:
    """Define a full set of profiling parameters."""

    grid: GridParams
    capture: _BaseCaptureParams

    def __post_init__(self) -> None:
        """Validate parameters."""
        if not isinstance(
            self.capture,
            (
                PointCountCaptureParams,
                PointTimeCaptureParams,
                ContinuousCaptureParams,
            ),
        ):
            raise ProfilerParamsInitError("Invalid capture mode.")
