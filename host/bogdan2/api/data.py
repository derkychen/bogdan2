"""Data processing utilities for the beam profiler."""

import csv
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Final, cast

import matplotlib.pyplot as plt
import matplotlib.tri as mtri
import numpy as np
import numpy.typing as npt
from mpl_toolkits.mplot3d import Axes3D

MIN_TRIANGULATION_NUM_POINTS: Final[int] = 3


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
        """Standard deviation of quantities."""
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

    def __init__(self, points: Iterable[BeamPoint]) -> None:
        """Initialize a profile."""
        self._points: tuple[BeamPoint, ...] = tuple(points)

        if not self._points:
            raise ValueError("Beam profile must contain at least one point.")

        if any(
            not np.isfinite(value)
            for point in self._points
            for value in (point.x_mm, point.y_mm, point.intensity)
        ):
            raise ValueError("Beam profile values must be finite.")

    @classmethod
    def from_csv(cls, path: str | Path) -> "BeamProfile":
        """Load a beam profile from a CSV."""
        points: list[BeamPoint] = []

        with Path(path).open(
            "r",
            encoding="utf-8",
            newline="",
        ) as file:
            reader = csv.DictReader(file)

            expected_fields = {"x_mm", "y_mm", "intensity"}

            if (
                reader.fieldnames is None
                or set(reader.fieldnames) != expected_fields
            ):
                raise ValueError(
                    "CSV must contain x_mm, y_mm, and intensity columns."
                )

            for row in reader:
                points.append(
                    BeamPoint(
                        x_mm=float(row["x_mm"]),
                        y_mm=float(row["y_mm"]),
                        intensity=float(row["intensity"]),
                    )
                )

        return cls(points)

    @classmethod
    def from_npz(cls, path: str | Path) -> "BeamProfile":
        """Load a beam profile from a NumPy archive."""
        with cast(
            np.lib.npyio.NpzFile[np.float64],
            np.load(
                path,
                allow_pickle=False,
            ),
        ) as data:
            xs_mm = np.asarray(data["x_mm"], dtype=np.float64)
            ys_mm = np.asarray(data["y_mm"], dtype=np.float64)
            intensities = np.asarray(
                data["intensity"],
                dtype=np.float64,
            )

        if not (xs_mm.shape == ys_mm.shape == intensities.shape):
            raise ValueError("Beam profile arrays have incompatible shapes.")

        if xs_mm.ndim != 1:
            raise ValueError("Beam profile arrays must be one-dimensional.")

        points = [
            BeamPoint(
                x_mm=float(x_mm),
                y_mm=float(y_mm),
                intensity=float(intensity),
            )
            for x_mm, y_mm, intensity in zip(
                xs_mm.flat,
                ys_mm.flat,
                intensities.flat,
                strict=True,
            )
        ]

        return cls(points)

    def plot2d(self) -> None:
        """Visualize beam profile in 2D."""
        xs_mm, ys_mm, intensities = self._arrays()

        triangulation = mtri.Triangulation(xs_mm, ys_mm)

        fig, ax = plt.subplots()

        heatmap = ax.tripcolor(
            triangulation,
            intensities,
            shading="gouraud",
            cmap="inferno",
        )

        _ = ax.scatter(
            xs_mm,
            ys_mm,
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

    def plot3d(self) -> None:
        """Visualize beam profile in 3D."""
        xs_mm, ys_mm, intensities = self._arrays()

        triangulation = mtri.Triangulation(xs_mm, ys_mm)

        fig = plt.figure()

        ax = fig.add_subplot(projection="3d")
        assert isinstance(ax, Axes3D)

        surface = ax.plot_trisurf(
            triangulation,
            intensities,
            cmap="inferno",
            linewidth=0.2,
            antialiased=True,
        )

        _ = ax.scatter(
            xs_mm,
            ys_mm,
            # NOTE: Matplotlib incorrectly types `zs` as `int`.
            intensities,  # pyright: ignore[reportArgumentType]
            s=10,
            c="black",
            alpha=0.5,
        )

        _ = ax.set_xlabel("x [mm]")
        _ = ax.set_ylabel("y [mm]")
        _ = ax.set_zlabel("Intensity")
        _ = ax.set_title("Beam Profile")

        _ = fig.colorbar(surface, ax=ax, label="Intensity", shrink=0.7)

        plt.show()

    def save_csv(self, path: str | Path) -> None:
        """Save the beam profile as a CSV."""
        with Path(path).open(
            "w",
            encoding="utf-8",
            newline="",
        ) as file:
            writer = csv.writer(file)

            writer.writerow(("x_mm", "y_mm", "intensity"))

            writer.writerows(
                (
                    point.x_mm,
                    point.y_mm,
                    point.intensity,
                )
                for point in self._points
            )

    def save_npz(self, path: str | Path) -> None:
        """Save the beam profile to a NumPy archive."""
        xs_mm, ys_mm, intensities = self._arrays()

        np.savez_compressed(
            path,
            x_mm=xs_mm,
            y_mm=ys_mm,
            intensity=intensities,
        )

    def _arrays(
        self,
    ) -> tuple[
        npt.NDArray[np.float64],
        npt.NDArray[np.float64],
        npt.NDArray[np.float64],
    ]:
        """Get the beam profile as three NumPy arrays."""
        xs_mm = np.asarray(
            [point.x_mm for point in self._points],
            dtype=np.float64,
        )
        ys_mm = np.asarray(
            [point.y_mm for point in self._points],
            dtype=np.float64,
        )
        intensities = np.asarray(
            [point.intensity for point in self._points],
            dtype=np.float64,
        )

        return xs_mm, ys_mm, intensities
