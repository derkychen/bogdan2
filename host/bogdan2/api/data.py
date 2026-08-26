"""Data processing utilities for the beam profiler."""

import csv
import math
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Final, Self, cast

import matplotlib.pyplot as plt
import matplotlib.tri as mtri
import numpy as np
import numpy.typing as npt
from mpl_toolkits.mplot3d import Axes3D

MIN_TRIANGULATION_NUM_POINTS: Final[int] = 3
MIN_BACKGROUND_POINTS_PER_CORNER: Final[int] = 4
BACKGROUND_CORNER_FRACTION: Final[float] = 0.1
ISO_THRESHOLD_SIGMA_MIN: Final[float] = 2.0
ISO_THRESHOLD_SIGMA_MAX: Final[float] = 4.0
ISO_THRESHOLD_SIGMA_DEFAULT: Final[float] = 4.0


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


# Named intermediates are for readability.
def _process_intensities(  # noqa: PLR0914
    xs_mm: npt.NDArray[np.float64],
    ys_mm: npt.NDArray[np.float64],
    intensities: npt.NDArray[np.float64],
    *,
    threshold_sigma: float,
) -> npt.NDArray[np.float64]:
    """Estimate background and apply an WinCamD-like noise threshold.

    Background is estimated through sampling the four corners of the
    rectangular grid, where readings are assumed to be background.
    """
    if not (
        math.isfinite(threshold_sigma)
        and ISO_THRESHOLD_SIGMA_MIN
        <= threshold_sigma
        <= ISO_THRESHOLD_SIGMA_MAX
    ):
        raise ValueError(
            "Threshold sigma must be between "
            + f"{ISO_THRESHOLD_SIGMA_MIN} and "
            + f"{ISO_THRESHOLD_SIGMA_MAX}."
        )

    assert xs_mm.shape == ys_mm.shape == intensities.shape, (
        "Arrays have incompatible shapes."
    )
    assert xs_mm.ndim == 1, "Axis arrays must be one-dimensional."
    assert np.all(np.isfinite(xs_mm))
    assert np.all(np.isfinite(ys_mm))

    x_min = float(np.min(xs_mm))
    x_max = float(np.max(xs_mm))
    y_min = float(np.min(ys_mm))
    y_max = float(np.max(ys_mm))

    x_span = x_max - x_min
    y_span = y_max - y_min

    if x_span <= 0.0 or y_span <= 0.0:
        raise ValueError("Beam profile must span both spatial dimensions.")

    x_margin = BACKGROUND_CORNER_FRACTION * x_span
    y_margin = BACKGROUND_CORNER_FRACTION * y_span

    left = xs_mm <= x_min + x_margin
    right = xs_mm >= x_max - x_margin
    bottom = ys_mm <= y_min + y_margin
    top = ys_mm >= y_max - y_margin

    corner_masks = (
        left & bottom,
        left & top,
        right & bottom,
        right & top,
    )

    if any(
        np.count_nonzero(mask) < MIN_BACKGROUND_POINTS_PER_CORNER
        for mask in corner_masks
    ):
        raise ValueError(
            "Beam profile contains too few samples in one or more "
            + "background regions."
        )

    background_mask = (
        corner_masks[0] | corner_masks[1] | corner_masks[2] | corner_masks[3]
    )

    background = intensities[background_mask]

    baseline = float(np.mean(background))
    noise_stddev = float(np.std(background))

    corrected = intensities - baseline
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
class BeamPoint:
    """Point on a beam profile.

    Attributes
    ----------
    x_mm : float
        x-coordinate of the point on the profile.
    y_mm : float
        y-coordinate of the point on the profile.
    intensity : float
        Integral of the voltage waveform captured. The magnitude of this
        quantity is not useful, it mainly serves as a relative metric.
    """

    x_mm: float
    y_mm: float
    intensity: float


@dataclass(frozen=True, slots=True, kw_only=True)
class BeamGeometry:
    """ISO 11146 second-moment beam geometry.

    Attributes
    ----------
    centroid_x_mm : float
        Beam centroid x-coordinate of in millimetres relative to the stage
        origin.
    centroid_y_mm : float
        Beam centroid y-coordinate of in millimetres relative to the stage
        origin.
    d4sigma_semimajor_mm : float
        Semi-major axis length in millimetres.
    d4sigma_semiminor_mm : float
        Semi-minor axis length in millimetres.
    orientation_rad : float
        Angle of the semi-major axis from the positive x-axis in [0, pi).
    ellipticity : float
        Ellipticity of the beam.
    """

    centroid_x_mm: float
    centroid_y_mm: float

    d4sigma_semimajor_mm: float
    d4sigma_semiminor_mm: float

    orientation_rad: float
    ellipticity: float

    @property
    def d4sigma_major_diameter_mm(self) -> float:
        """D4sigma major-axis diameter.

        Returns
        -------
        float
            Semi-major axis length multiplied by two.
        """
        return 2.0 * self.d4sigma_semimajor_mm

    @property
    def d4sigma_minor_diameter_mm(self) -> float:
        """D4sigma minor-axis diameter.

        Returns
        -------
        float
            Semi-minor axis length multiplied by two.
        """
        return 2.0 * self.d4sigma_semiminor_mm


class BeamProfile:
    """Data processing functionality for beam profiles."""

    def __init__(self, points: Iterable[BeamPoint]) -> None:
        """Initialize a profile.

        Validates input and creates an immutable beam profile by storing the
        provided `BeamPoint` objects in a tuple.

        Parameters
        ----------
        points : iterable[BeamPoint]
            An iterable object, usually a list of `BeamPoint` objects.

        Raises
        ------
        ValueError
            If the beam profile contains too few points or if the profile
            contains data that is not finite.
        """
        self._points: tuple[BeamPoint, ...] = tuple(points)

        if not self._points:
            raise ValueError("Beam profile must contain at least one point.")

        if any(
            not math.isfinite(value)
            for point in self._points
            for value in (point.x_mm, point.y_mm, point.intensity)
        ):
            raise ValueError("Beam profile values must be finite.")

    @classmethod
    def from_csv(cls, path: str | Path) -> Self:
        """Load a beam profile from a CSV.

        Parameters
        ----------
        path : str or Path
            File path of the CSV.

        Returns
        -------
        Self
            A `BeamProfile` instance from the loaded data.

        Raises
        ------
        ValueError
            If the CSV does not contain only the required columns.
        """
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
    def from_npz(cls, path: str | Path) -> Self:
        """Load a beam profile from a NumPy archive.

        Parameters
        ----------
        path : str or Path
            File path of the NumPy archive.

        Returns
        -------
        Self
            A `BeamProfile` instance from the loaded data.

        Raises
        ------
        ValueError
            If the loaded data contains incompatible or non-one-dimensional
            arrays.
        """
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

    @property
    def points(self) -> tuple[BeamPoint, ...]:
        """The points in the beam profile.

        Returns
        -------
        tuple of BeamPoints
            Points in the profile.
        """
        return self._points

    def plot(self) -> None:
        """Visualize the beam profile in 2D and 3D side-by-side.

        Uses Delauney triangulation to interpolate between points. Equal
        scaling of the x- and y-axes is enforced. The z-axis is not constrained
        by scaling.
        """
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
        """Save the beam profile as a CSV.

        Parameters
        ----------
        path: str | Path
            The path to which to save the beam profile CSV. This path is
            relative to the current working directory of the caller of the
            function.
        """
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
        """Save the beam profile to a NumPy archive.

        Parameters
        ----------
        path: str | Path
            The path to which to save the beam profile NumPy archive. This path
            is relative to the current working directory of the caller of the
            function.
        """
        xs_mm, ys_mm, intensities = self._arrays()

        np.savez_compressed(
            path,
            x_mm=xs_mm,
            y_mm=ys_mm,
            intensity=intensities,
        )

    # Named intermediates are for readability.
    def geometry(  # noqa: PLR0914
        self,
        *,
        threshold_sigma: float = ISO_THRESHOLD_SIGMA_DEFAULT,
    ) -> BeamGeometry:
        """Calculate ISO 11146 second-moment beam geometry.

        Background thresholding is done similar to in the WinCamD, where four
        corners are sampled and assumed to be background.

        The intensities are linearly interpolated over a Delauney triangulation
        before spatial moments are integrated. From these, all quantities in
        the `BeamGeometry` object are derived.

        It is important to note that, while this calculation follows ISO
        second-moment definitions, it does not establish compliance with ISO
        11146.

        Parameters
        ----------
        threshold_sigma : float, default=4.0
            Cutoff of the beam; as per ISO this must be between 2.0 and 4.0.

        Returns
        -------
        BeamGeometry
            ISO 11146 geometry of the beam.

        Raises
        ------
        ValueError
            If the beam has too few points, insufficient number of points in
            background sampling regions, contains no positive signal, zero
            spatial extent.
        """
        xs_mm, ys_mm, intensities = self._arrays()

        processed = _process_intensities(
            xs_mm,
            ys_mm,
            intensities,
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
