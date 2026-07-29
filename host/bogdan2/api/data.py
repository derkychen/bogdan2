"""Data processing utilities for the beam profiler."""

from dataclasses import dataclass
from pathlib import Path

import numpy as np
from numpy.typing import NDArray


@dataclass(frozen=True, slots=True, kw_only=True)
class Reading:
    """All measured quantities are readings from the PicoScope."""

    data: NDArray[np.float64]
    interval_s: float

    @property
    def mean(self) -> float:
        """Return the average of a waveform."""
        return float(np.mean(self.data))

    @property
    def median(self) -> float:
        """Return the median of a waveform."""
        return float(np.median(self.data))

    @property
    def integral(self) -> float:
        """Return the integral of a waveform by method of trapezoids."""
        return float(
            np.trapezoid(self.data, x=None, dx=self.interval_s, axis=-1)
        )


@dataclass(frozen=True, slots=True, kw_only=True)
class Position:
    """Position of a point on the beam profile."""

    x_mm: float
    y_mm: float


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamPoint:
    """Point on a beam profile."""

    position: Position
    intensity: float


class Profile:
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
