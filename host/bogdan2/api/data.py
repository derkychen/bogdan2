"""Data processing utilities for the beam profiler."""

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import numpy.typing as npt


@dataclass(frozen=True, slots=True, kw_only=True)
class Reading:
    """Abstraction for raw readings captured from the oscilloscope."""

    vals: npt.NDArray[np.float64]

    @property
    def mean(self) -> float:
        """Mean of quantities."""
        return float(np.mean(self.vals))

    @property
    def median(self) -> float:
        """Median of quantities."""
        return float(np.median(self.vals))

    @property
    def stddev(self) -> float:
        """Median of quantities."""
        return float(np.std(self.vals))

    def integral(self, interval: float) -> float:
        """Integral of a waveform by trapezoidal method."""
        return float(np.trapezoid(self.vals, x=None, dx=interval, axis=-1))


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamPoint:
    """Point on a beam profile."""

    x_mm: float
    y_mm: float
    intensity: float


class BeamProfile:
    """Data processing functionality for beam profiles."""

    def __init__(self, points: list[BeamPoint]) -> None:
        """Initialize a profile."""
        self._points: list[BeamPoint] = points

    def save(self, path: Path) -> None:
        """Save profile data to a location."""
        # TODO: Implement.
        pass

    def plot(self) -> None:
        """Visualize beam profile."""
        # TODO: Implement.
        pass
