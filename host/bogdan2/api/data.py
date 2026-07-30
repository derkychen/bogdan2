"""Data processing utilities for the beam profiler."""

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import numpy.typing as npt


@dataclass(frozen=True, slots=True, kw_only=True)
class Quantity:
    """Abstraction for quantities."""

    value: float
    unit: str


@dataclass(frozen=True, slots=True, kw_only=True)
class Sequence:
    """Abstraction for data (multiple quantities)."""

    values: npt.NDArray[np.float64]
    unit: str

    @property
    def mean(self) -> float:
        """Mean of a waveform."""
        return float(np.mean(self.values))

    @property
    def median(self) -> float:
        """Median of a waveform."""
        return float(np.median(self.values))


@dataclass(frozen=True, slots=True, kw_only=True)
class TimeSeries:
    """All measured quantities are readings from the PicoScope."""

    data: Sequence
    interval: Quantity | None

    @property
    def integral(self) -> Quantity:
        """The integral of a waveform by trapezoidal method."""
        assert self.interval is not None, (
            "Data interval must be set for integration."
        )

        return Quantity(
            value=float(
                np.trapezoid(
                    self.data.values, x=None, dx=self.interval.value, axis=-1
                )
            ),
            unit=f"{self.data.unit} * {self.interval.unit}",
        )


@dataclass(frozen=True, slots=True, kw_only=True)
class Position:
    """Position of a point on the beam profile."""

    x: TimeSeries
    y: TimeSeries


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamPoint:
    """Point on a beam profile."""

    position: Position
    intensity: TimeSeries


class BeamProfile:
    """Data processing functionality for beam profiles."""

    def __init__(self, points: list[BeamPoint]) -> None:
        """Initialize a profile."""
        # TODO: Implement.
        pass

    def save(self, path: Path) -> None:
        """Save profile data to a location."""
        # TODO: Implement.
        pass

    def plot(self) -> None:
        """Visualize beam profile."""
        # TODO: Implement.
        pass
