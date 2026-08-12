"""Data processing utilities for the beam profiler."""

from dataclasses import dataclass

import matplotlib.pyplot as plt
import matplotlib.tri as mtri
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

    def plot2d(self) -> None:
        """Visualize beam profile in 2D.

        Uses Delauney triangulation to interpolate between points.
        """
        x = np.array([p.x_mm for p in self._points])
        y = np.array([p.y_mm for p in self._points])
        intensity = np.array([p.intensity for p in self._points])

        triangulation = mtri.Triangulation(x, y)

        fig, ax = plt.subplots()

        heatmap = ax.tripcolor(
            triangulation,
            intensity,
            shading="gouraud",
            cmap="inferno",
        )

        _ = ax.scatter(
            x,
            y,
            s=10,
            c="black",
            alpha=0.5,
        )

        _ = ax.set_xlabel("x [mm]")
        _ = ax.set_ylabel("y [mm]")
        _ = ax.set_title("Beam Profile")

        ax.set_aspect("equal")

        _ = fig.colorbar(heatmap, ax=ax, label="Intensity")

        plt.show()
