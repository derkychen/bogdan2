"""Data processing utilities for the beam profiler."""

import csv
import math
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


@dataclass(frozen=True, slots=True)
class _Moments:
    m00: float
    m10: float
    m01: float
    m20: float
    m11: float
    m02: float


def _triangulation(
    xs: npt.NDArray[np.float64], ys: npt.NDArray[np.float64]
) -> mtri.Triangulation:
    """Get a triangulation from coordinates of beam samples."""
    assert xs.shape == ys.shape, "Axis arrays have incompatible shapes."
    assert xs.ndim == 1, "Axis arrays must be one-dimensional."
    assert np.all(np.isfinite(xs))
    assert np.all(np.isfinite(ys))

    if len(xs) < MIN_TRIANGULATION_NUM_POINTS:
        raise ValueError(
            "At least three beam profile points are required for"
            + "triangulation."
        )

    return mtri.Triangulation(xs, ys)


def _process_intensities(
    intensities: npt.NDArray[np.float64],
    *,
    background: float,
    noise_stddev: float | None,
    threshold_sigma: float,
) -> npt.NDArray[np.float64]:
    """Subtract background and apply noise threshold."""
    if not math.isfinite(background):
        raise ValueError("Background must be finite.")

    if threshold_sigma < 0.0 or not math.isfinite(threshold_sigma):
        raise ValueError("Threshold sigma must be finite and non-negative.")

    corrected = intensities - background

    if noise_stddev is None:
        return np.maximum(corrected, 0.0)

    if noise_stddev < 0.0 or not math.isfinite(noise_stddev):
        raise ValueError(
            "Noise standard deviation must be finite and non-negative."
        )

    threshold = threshold_sigma * noise_stddev

    return np.where(
        corrected > threshold,
        corrected,
        0.0,
    )


def _integrated_moments(
    triangulation: mtri.Triangulation,
    xs_mm: npt.NDArray[np.float64],
    ys_mm: npt.NDArray[np.float64],
    intensities: npt.NDArray[np.float64],
) -> _Moments:
    """Integrate spatial moments of the interpolated beam profile."""
    triangles = np.asarray(
        triangulation.triangles,
        dtype=np.intp,
    )

    triangle_xs = xs_mm[triangles]
    triangle_ys = ys_mm[triangles]
    triangle_intensities = intensities[triangles]

    areas = 0.5 * np.abs(
        (triangle_xs[:, 1] - triangle_xs[:, 0])
        * (triangle_ys[:, 2] - triangle_ys[:, 0])
        - (triangle_xs[:, 2] - triangle_xs[:, 0])
        * (triangle_ys[:, 1] - triangle_ys[:, 0])
    )

    barycentric = np.asarray(
        (
            (1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0),
            (0.6, 0.2, 0.2),
            (0.2, 0.6, 0.2),
            (0.2, 0.2, 0.6),
        ),
        dtype=np.float64,
    )

    quadrature_weights = np.asarray(
        (
            -27.0 / 48.0,
            25.0 / 48.0,
            25.0 / 48.0,
            25.0 / 48.0,
        ),
        dtype=np.float64,
    )

    qx = triangle_xs @ barycentric.T
    qy = triangle_ys @ barycentric.T
    qi = triangle_intensities @ barycentric.T

    weighted_intensity = (
        areas[:, np.newaxis] * quadrature_weights[np.newaxis, :] * qi
    )

    return _Moments(
        m00=float(np.sum(weighted_intensity)),
        m10=float(np.sum(weighted_intensity * qx)),
        m01=float(np.sum(weighted_intensity * qy)),
        m20=float(np.sum(weighted_intensity * qx * qx)),
        m11=float(np.sum(weighted_intensity * qx * qy)),
        m02=float(np.sum(weighted_intensity * qy * qy)),
    )


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


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamGeometry:
    """ISO-style second-moment beam geometry."""

    centroid_x_mm: float
    centroid_y_mm: float

    d4sigma_semimajor_mm: float
    d4sigma_semiminor_mm: float

    orientation_rad: float
    ellipticity: float

    @property
    def d4sigma_major_diameter_mm(self) -> float:
        """D4sigma major-axis diameter."""
        return 2.0 * self.d4sigma_semimajor_mm

    @property
    def d4sigma_minor_diameter_mm(self) -> float:
        """D4sigma minor-axis diameter."""
        return 2.0 * self.d4sigma_semiminor_mm


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

    def plot(self) -> None:
        """Visualize the beam profile in 2D and 3D."""
        xs_mm, ys_mm, intensities = self._arrays()
        triangulation = _triangulation(xs_mm, ys_mm)

        fig = plt.figure(figsize=(12, 5))

        ax2d = fig.add_subplot(1, 2, 1)

        heatmap = ax2d.tripcolor(
            triangulation,
            intensities,
            shading="gouraud",
            cmap="inferno",
        )

        _ = ax2d.set_xlabel("x [mm]")
        _ = ax2d.set_ylabel("y [mm]")
        _ = ax2d.set_title("Beam Profile — 2D")

        ax2d.set_aspect("equal", adjustable="box")

        _ = fig.colorbar(
            heatmap,
            ax=ax2d,
            label="Intensity",
        )

        ax3d = fig.add_subplot(
            1,
            2,
            2,
            projection="3d",
        )
        assert isinstance(ax3d, Axes3D)

        surface = ax3d.plot_trisurf(
            triangulation,
            intensities,
            cmap="inferno",
            linewidth=0.2,
            antialiased=True,
        )

        _ = ax3d.set_xlabel("x [mm]")
        _ = ax3d.set_ylabel("y [mm]")
        _ = ax3d.set_zlabel("Intensity")
        _ = ax3d.set_title("Beam Profile — 3D")

        ax3d.set_aspect(
            # NOTE: "equalxy" is supported but Matplotlib types it incorrectly.
            "equalxy",  # pyright: ignore[reportArgumentType]
            adjustable="box",
        )

        _ = fig.colorbar(
            surface,
            ax=ax3d,
            label="Intensity",
            shrink=0.7,
        )

        fig.tight_layout()
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

    def geometry(  # noqa: PLR0914  # Named intermediates are for readability.
        self,
        background: float = 0.0,
        noise_stddev: float | None = None,
        threshold_sigma: float = 3.0,
    ) -> BeamGeometry:
        """Calculate ISO-style second-moment beam geometry."""
        xs_mm, ys_mm, intensities = self._arrays()

        processed = _process_intensities(
            intensities,
            background=background,
            noise_stddev=noise_stddev,
            threshold_sigma=threshold_sigma,
        )

        triangulation = _triangulation(xs_mm, ys_mm)

        moments = _integrated_moments(
            triangulation,
            xs_mm,
            ys_mm,
            processed,
        )

        if moments.m00 <= 0.0:
            raise ValueError("Beam profile contains no positive signal.")

        centroid_x_mm = moments.m10 / moments.m00
        centroid_y_mm = moments.m01 / moments.m00

        sigma_xx = moments.m20 / moments.m00 - centroid_x_mm**2
        sigma_xy = moments.m11 / moments.m00 - centroid_x_mm * centroid_y_mm
        sigma_yy = moments.m02 / moments.m00 - centroid_y_mm**2

        covariance = np.asarray(
            (
                (sigma_xx, sigma_xy),
                (sigma_xy, sigma_yy),
            ),
            dtype=np.float64,
        )

        eigenvalues, eigenvectors = cast(
            tuple[
                npt.NDArray[np.float64],
                npt.NDArray[np.float64],
            ],
            np.linalg.eigh(covariance),
        )

        minor_variance = float(eigenvalues.flat[0])
        major_variance = float(eigenvalues.flat[1])

        tolerance = (
            100.0
            * np.finfo(np.float64).eps
            * max(
                abs(sigma_xx),
                abs(sigma_xy),
                abs(sigma_yy),
            )
        )

        if minor_variance < -tolerance:
            raise ValueError(
                "Beam covariance matrix has a negative eigenvalue."
            )

        minor_variance = max(minor_variance, 0.0)

        if major_variance <= 0.0:
            raise ValueError("Beam profile has zero spatial extent.")

        major_vector = np.asarray(
            eigenvectors[:, 1],
            dtype=np.float64,
        )

        major_x = float(major_vector.flat[0])
        major_y = float(major_vector.flat[1])

        orientation_rad = math.atan2(major_y, major_x) % math.pi

        semimajor_mm = 2.0 * math.sqrt(major_variance)
        semiminor_mm = 2.0 * math.sqrt(minor_variance)

        return BeamGeometry(
            centroid_x_mm=centroid_x_mm,
            centroid_y_mm=centroid_y_mm,
            d4sigma_semimajor_mm=semimajor_mm,
            d4sigma_semiminor_mm=semiminor_mm,
            orientation_rad=orientation_rad,
            ellipticity=semiminor_mm / semimajor_mm,
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
