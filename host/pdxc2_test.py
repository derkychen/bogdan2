"""Safer ctypes wrapper and diagnostic utility for the Thorlabs PDXC2.

The diagnostic is read-only unless --write-analog is supplied.  Even then, the
controller is left in Manual trigger mode unless --arm-analog-rising is also
supplied.

Requires Python 3.10+ on Windows and a matching-bitness Kinesis installation.
"""

from __future__ import annotations

import argparse
import ctypes
import math
import os
import struct
import sys
import time
from collections.abc import Callable
from typing import Final, ParamSpec, TypeVar, cast

P = ParamSpec("P")
R = TypeVar("R")

DEFAULT_KINESIS_DIR: Final[str] = r"C:\Program Files\Thorlabs\Kinesis"
KINESIS_DLL_FILE: Final[str] = "Thorlabs.MotionControl.Benchtop.Piezo.dll"

POLLING_INTERVAL_MS: Final[int] = 200
OPEN_SETTLING_TIME_S: Final[float] = 0.250
SETTINGS_SETTLING_TIME_S: Final[float] = 0.500
ENABLE_SETTLING_TIME_S: Final[float] = 0.500
TRIGGER_REQUEST_SETTLING_TIME_S: Final[float] = 0.300
TRIGGER_WRITE_SETTLING_TIME_S: Final[float] = 0.150

TRIGGER_MODE_MANUAL: Final[int] = 0x00
TRIGGER_MODE_ANALOG_RISING: Final[int] = 0x01
VALID_TRIGGER_MODES: Final[set[int]] = set(range(0x00, 0x07))

_FT_STATUS_NAMES: Final[dict[int, str]] = {
    0x00: "FT_OK",
    0x01: "FT_InvalidHandle",
    0x02: "FT_DeviceNotFound",
    0x03: "FT_DeviceNotOpened",
    0x04: "FT_IOError",
    0x05: "FT_InsufficientResources",
    0x06: "FT_InvalidParameter",
    0x07: "FT_DeviceNotPresent",
    0x08: "FT_IncorrectDevice",
}


class KinesisStatusFailure(RuntimeError):
    """Raised when a Thorlabs Kinesis C API call reports failure."""


def _check_err_status_code(
    func: Callable[P, int], *args: P.args, **kwargs: P.kwargs
) -> int:
    """Call a Kinesis function that returns a 16-bit status code."""
    ret = int(func(*args, **kwargs))
    if ret != 0:
        unsigned = ret & 0xFFFF
        label = _FT_STATUS_NAMES.get(unsigned, "unknown status")
        raise KinesisStatusFailure(
            f"{func.__name__} failed: {label}, code={ret} (0x{unsigned:04X})."
        )
    return ret


def _check_err_bool(
    func: Callable[P, int | bool], *args: P.args, **kwargs: P.kwargs
) -> None:
    """Call a Kinesis function that returns a C++ bool."""
    if not bool(func(*args, **kwargs)):
        raise KinesisStatusFailure(f"{func.__name__} failed (returned false).")


class PDXC2_ClosedLoopParameters(ctypes.Structure):
    """Mirrors PDXC2_ClosedLoopParameters in the Kinesis C header."""

    _pack_ = 1
    _fields_ = [
        ("RefSpeed", ctypes.c_uint32),
        ("Proportional", ctypes.c_uint32),
        ("Integral", ctypes.c_uint32),
        ("Differential", ctypes.c_uint32),
        ("Acceleration", ctypes.c_uint32),
    ]


class PDXC2TriggerParams(ctypes.Structure):
    """Mirrors PDXC2_TriggerParams in the Kinesis C header."""

    _pack_ = 1
    _fields_ = [
        ("RiseFixedStep", ctypes.c_int32),
        ("FallFixedStep", ctypes.c_int32),
        ("RisePosition1", ctypes.c_int32),
        ("FallPosition1", ctypes.c_int32),
        ("RisePosition2", ctypes.c_int32),
        ("FallPosition2", ctypes.c_int32),
        ("AnalogInGain", ctypes.c_float),
        ("AnalogInOffset", ctypes.c_float),
        ("AnalogOutGain", ctypes.c_float),
        ("AnalogOutOffset", ctypes.c_float),
    ]

    def as_dict(self) -> dict[str, int | float]:
        return {name: getattr(self, name) for name, _ctype in self._fields_}


# Fail immediately if the local ABI definition is not what the header requires.
assert ctypes.sizeof(PDXC2TriggerParams) == 40
assert [getattr(PDXC2TriggerParams, name).offset for name, _ in PDXC2TriggerParams._fields_] == list(
    range(0, 40, 4)
)
assert ctypes.sizeof(PDXC2_ClosedLoopParameters) == 20


class Controller:
    """Small, verifiable wrapper around the PDXC2 Kinesis C API."""

    def __init__(
        self,
        serial_num: bytes | str,
        *,
        kinesis_dir: str = DEFAULT_KINESIS_DIR,
        trigger_request_settling_s: float = TRIGGER_REQUEST_SETTLING_TIME_S,
    ) -> None:
        if os.name != "nt":
            raise OSError("The Kinesis DLL wrapper must be run on Windows.")

        self._serial_bytes = self._normalize_serial(serial_num)
        self._serial_num = ctypes.c_char_p(self._serial_bytes)
        self._kinesis_dir = os.path.abspath(kinesis_dir)
        self._trigger_request_settling_s = float(trigger_request_settling_s)
        if self._trigger_request_settling_s <= 0:
            raise ValueError("trigger_request_settling_s must be positive.")

        dll_path = os.path.join(self._kinesis_dir, KINESIS_DLL_FILE)
        if not os.path.isfile(dll_path):
            raise FileNotFoundError(f"Kinesis DLL not found: {dll_path}")

        # On Python 3.8+, the full path to the top-level DLL does not guarantee
        # that Windows can resolve that DLL's dependent Kinesis DLLs.
        self._dll_dir_handle = os.add_dll_directory(self._kinesis_dir)
        self._lib = ctypes.CDLL(dll_path)  # Kinesis exports these functions as __cdecl.
        self._set_function_prototypes()

        self._opened = False
        self._polling = False
        self._position = ctypes.c_int32(0)

    @staticmethod
    def _normalize_serial(serial_num: bytes | str) -> bytes:
        raw = serial_num.encode("ascii") if isinstance(serial_num, str) else bytes(serial_num)
        raw = raw.strip()
        if raw.upper().startswith(b"SN:"):
            raw = raw[3:].strip()
        if not raw or not raw.isdigit():
            raise ValueError(
                "serial_num must contain the numeric serial only, for example b'112000001'."
            )
        return raw

    def _set_function_prototypes(self) -> None:
        lib = self._lib

        lib.TLI_BuildDeviceList.argtypes = []
        lib.TLI_BuildDeviceList.restype = ctypes.c_short

        lib.PDXC2_Open.argtypes = [ctypes.c_char_p]
        lib.PDXC2_Open.restype = ctypes.c_short
        lib.PDXC2_Close.argtypes = [ctypes.c_char_p]
        lib.PDXC2_Close.restype = None

        lib.PDXC2_StartPolling.argtypes = [ctypes.c_char_p, ctypes.c_int]
        lib.PDXC2_StartPolling.restype = ctypes.c_bool
        lib.PDXC2_StopPolling.argtypes = [ctypes.c_char_p]
        lib.PDXC2_StopPolling.restype = None

        lib.PDXC2_Enable.argtypes = [ctypes.c_char_p]
        lib.PDXC2_Enable.restype = ctypes.c_short

        lib.PDXC2_RequestSettings.argtypes = [ctypes.c_char_p]
        lib.PDXC2_RequestSettings.restype = ctypes.c_short

        lib.PDXC2_GetSoftwareVersion.argtypes = [ctypes.c_char_p]
        lib.PDXC2_GetSoftwareVersion.restype = ctypes.c_uint32

        lib.PDXC2_RequestExternalTriggerConfig.argtypes = [ctypes.c_char_p]
        lib.PDXC2_RequestExternalTriggerConfig.restype = ctypes.c_short
        lib.PDXC2_GetExternalTriggerConfig.argtypes = [ctypes.c_char_p]
        lib.PDXC2_GetExternalTriggerConfig.restype = ctypes.c_uint16
        lib.PDXC2_SetExternalTriggerConfig.argtypes = [
            ctypes.c_char_p,
            ctypes.c_uint16,
        ]
        lib.PDXC2_SetExternalTriggerConfig.restype = ctypes.c_short

        lib.PDXC2_RequestExternalTriggerParams.argtypes = [ctypes.c_char_p]
        lib.PDXC2_RequestExternalTriggerParams.restype = ctypes.c_short
        lib.PDXC2_GetExternalTriggerParams.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(PDXC2TriggerParams),
        ]
        lib.PDXC2_GetExternalTriggerParams.restype = ctypes.c_short
        lib.PDXC2_SetExternalTriggerParams.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(PDXC2TriggerParams),
        ]
        lib.PDXC2_SetExternalTriggerParams.restype = ctypes.c_short

        lib.PDXC2_RequestPosition.argtypes = [ctypes.c_char_p]
        lib.PDXC2_RequestPosition.restype = ctypes.c_short
        lib.PDXC2_GetPosition.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_int32),
        ]
        lib.PDXC2_GetPosition.restype = ctypes.c_short

        lib.PDXC2_SetClosedLoopParams.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(PDXC2_ClosedLoopParameters),
        ]
        lib.PDXC2_SetClosedLoopParams.restype = ctypes.c_short

        lib.PDXC2_PersistSettings.argtypes = [ctypes.c_char_p]
        lib.PDXC2_PersistSettings.restype = ctypes.c_bool

    @property
    def serial(self) -> str:
        return self._serial_bytes.decode("ascii")

    def open_and_enable(self) -> None:
        if self._opened:
            return

        _check_err_status_code(self._lib.TLI_BuildDeviceList)
        _check_err_status_code(self._lib.PDXC2_Open, self._serial_num)
        self._opened = True

        try:
            time.sleep(OPEN_SETTLING_TIME_S)

            # Populate the DLL-side settings cache before any Get... call.
            _check_err_status_code(self._lib.PDXC2_RequestSettings, self._serial_num)
            time.sleep(SETTINGS_SETTLING_TIME_S)

            _check_err_bool(
                self._lib.PDXC2_StartPolling,
                self._serial_num,
                POLLING_INTERVAL_MS,
            )
            self._polling = True
            time.sleep(ENABLE_SETTLING_TIME_S)

            _check_err_status_code(self._lib.PDXC2_Enable, self._serial_num)
            time.sleep(ENABLE_SETTLING_TIME_S)
        except BaseException:
            self.close()
            raise

    # Compatibility with the original wrapper's method name.
    enable = open_and_enable

    def get_software_version_raw(self) -> int:
        self._require_open()
        return int(self._lib.PDXC2_GetSoftwareVersion(self._serial_num))

    def _request_trigger_mode_once(self) -> int:
        _check_err_status_code(
            self._lib.PDXC2_RequestExternalTriggerConfig,
            self._serial_num,
        )
        time.sleep(self._trigger_request_settling_s)
        mode = int(self._lib.PDXC2_GetExternalTriggerConfig(self._serial_num))
        if mode not in VALID_TRIGGER_MODES:
            raise KinesisStatusFailure(
                f"PDXC2_GetExternalTriggerConfig returned unknown mode 0x{mode:04X}."
            )
        return mode

    def get_trigger_mode(self, *, verify_stable: bool = True) -> int:
        """Request a fresh trigger mode instead of trusting an old DLL cache value."""
        self._require_open()
        first = self._request_trigger_mode_once()
        if not verify_stable:
            return first
        second = self._request_trigger_mode_once()
        if first == second:
            return second
        third = self._request_trigger_mode_once()
        if second != third:
            raise KinesisStatusFailure(
                f"Trigger mode did not stabilize: {first:#06x}, {second:#06x}, {third:#06x}."
            )
        return third

    def _request_trigger_params_once(self) -> PDXC2TriggerParams:
        _check_err_status_code(
            self._lib.PDXC2_RequestExternalTriggerParams,
            self._serial_num,
        )
        time.sleep(self._trigger_request_settling_s)
        params = PDXC2TriggerParams()
        _check_err_status_code(
            self._lib.PDXC2_GetExternalTriggerParams,
            self._serial_num,
            ctypes.byref(params),
        )
        return params

    def get_trigger_params(self, *, verify_stable: bool = True) -> PDXC2TriggerParams:
        """Request fresh trigger parameters and optionally require stable snapshots."""
        self._require_open()
        first = self._request_trigger_params_once()
        if not verify_stable:
            return first
        second = self._request_trigger_params_once()
        if bytes(first) == bytes(second):
            return second
        third = self._request_trigger_params_once()
        if bytes(second) != bytes(third):
            raise KinesisStatusFailure(
                "Trigger parameters did not stabilize across three device requests."
            )
        return third

    def set_trigger_mode(self, mode: int, *, verify: bool = True) -> int:
        self._require_open()
        mode = int(mode)
        if mode not in VALID_TRIGGER_MODES:
            raise ValueError(f"Invalid trigger mode: {mode:#x}")

        _check_err_status_code(
            self._lib.PDXC2_SetExternalTriggerConfig,
            self._serial_num,
            mode,
        )
        time.sleep(TRIGGER_WRITE_SETTLING_TIME_S)

        if not verify:
            return mode
        actual = self.get_trigger_mode()
        if actual != mode:
            raise KinesisStatusFailure(
                f"Trigger mode verification failed: requested {mode:#06x}, read {actual:#06x}."
            )
        return actual

    def set_to_analog_rising_trigger_mode(self) -> None:
        self.set_trigger_mode(TRIGGER_MODE_ANALOG_RISING)

    def set_analog_rising_trigger_params(
        self,
        analog_in_gain: float,
        analog_in_offset: float,
        analog_out_gain: float,
        analog_out_offset: float,
        *,
        verify: bool = True,
    ) -> PDXC2TriggerParams:
        """Update only the four analog fields, preserving all six fixed-trigger fields."""
        self._require_open()
        params = self.get_trigger_params()

        expected = {
            "AnalogInGain": ctypes.c_float(analog_in_gain).value,
            "AnalogInOffset": ctypes.c_float(analog_in_offset).value,
            "AnalogOutGain": ctypes.c_float(analog_out_gain).value,
            "AnalogOutOffset": ctypes.c_float(analog_out_offset).value,
        }
        for field, value in expected.items():
            setattr(params, field, value)

        _check_err_status_code(
            self._lib.PDXC2_SetExternalTriggerParams,
            self._serial_num,
            ctypes.byref(params),
        )
        time.sleep(TRIGGER_WRITE_SETTLING_TIME_S)

        if not verify:
            return params

        actual = self.get_trigger_params()
        mismatches: list[str] = []
        for field, wanted in expected.items():
            got = float(getattr(actual, field))
            if not math.isclose(got, wanted, rel_tol=1e-6, abs_tol=1e-6):
                mismatches.append(f"{field}: requested {wanted!r}, read {got!r}")
        if mismatches:
            raise KinesisStatusFailure(
                "Trigger parameter verification failed: " + "; ".join(mismatches)
            )
        return actual

    def configure_analog_rising(
        self,
        analog_in_gain: float,
        analog_in_offset: float,
        analog_out_gain: float,
        analog_out_offset: float,
        *,
        arm: bool = False,
        persist: bool = False,
    ) -> PDXC2TriggerParams:
        """Safely write analog parameters while the external trigger is disarmed."""
        self.set_trigger_mode(TRIGGER_MODE_MANUAL)
        actual = self.set_analog_rising_trigger_params(
            analog_in_gain,
            analog_in_offset,
            analog_out_gain,
            analog_out_offset,
        )
        if arm:
            self.set_trigger_mode(TRIGGER_MODE_ANALOG_RISING)
        if persist:
            self.persist_settings()
        return actual

    def get_position(self) -> int:
        self._require_open()
        _check_err_status_code(self._lib.PDXC2_RequestPosition, self._serial_num)
        time.sleep(self._trigger_request_settling_s)
        _check_err_status_code(
            self._lib.PDXC2_GetPosition,
            self._serial_num,
            ctypes.byref(self._position),
        )
        return int(self._position.value)

    def set_closedloop_params(
        self,
        refspeed: int = 10_000_000,
        proportional: int = 8192,
        integral: int = 8192,
        differential: int = 0,
        acceleration: int = 50_000_000,
    ) -> None:
        self._require_open()
        params = PDXC2_ClosedLoopParameters(
            refspeed,
            proportional,
            integral,
            differential,
            acceleration,
        )
        _check_err_status_code(
            self._lib.PDXC2_SetClosedLoopParams,
            self._serial_num,
            ctypes.byref(params),
        )

    def persist_settings(self) -> None:
        self._require_open()
        _check_err_bool(self._lib.PDXC2_PersistSettings, self._serial_num)

    def _require_open(self) -> None:
        if not self._opened:
            raise RuntimeError("Controller is not open; call open_and_enable() first.")

    def close(self) -> None:
        if self._polling:
            try:
                self._lib.PDXC2_StopPolling(self._serial_num)
            finally:
                self._polling = False
        if self._opened:
            try:
                self._lib.PDXC2_Close(self._serial_num)
            finally:
                self._opened = False

    def __enter__(self) -> Controller:
        self.open_and_enable()
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
        self.close()


def _format_mode(mode: int) -> str:
    names = {
        0x00: "Manual",
        0x01: "AnalogRising",
        0x02: "AnalogFalling",
        0x03: "FixedStepRising",
        0x04: "FixedStepFalling",
        0x05: "TwoPositionRising",
        0x06: "TwoPositionFalling",
    }
    return f"{names.get(mode, 'Unknown')} (0x{mode:04X})"


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read or safely test PDXC2 external-trigger settings through Kinesis."
    )
    parser.add_argument("serial", help="Numeric PDXC2 serial, with or without the SN: prefix")
    parser.add_argument("--kinesis-dir", default=DEFAULT_KINESIS_DIR)
    parser.add_argument(
        "--request-delay",
        type=float,
        default=TRIGGER_REQUEST_SETTLING_TIME_S,
        help="Seconds to wait between Request... and Get... (default: %(default)s)",
    )
    parser.add_argument(
        "--write-analog",
        nargs=4,
        type=float,
        metavar=("IN_GAIN", "IN_OFFSET", "OUT_GAIN", "OUT_OFFSET"),
        help="Write and verify the four analog trigger fields. Leaves mode Manual by default.",
    )
    parser.add_argument(
        "--arm-analog-rising",
        action="store_true",
        help="After a successful write, set trigger mode to AnalogRising.",
    )
    parser.add_argument(
        "--persist",
        action="store_true",
        help="Call PDXC2_PersistSettings after a successful write.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = _build_arg_parser().parse_args(argv)
    if args.arm_analog_rising and args.write_analog is None:
        raise SystemExit("--arm-analog-rising requires --write-analog")
    if args.persist and args.write_analog is None:
        raise SystemExit("--persist requires --write-analog")

    print(f"Python: {sys.version.split()[0]}, process bitness: {struct.calcsize('P') * 8}-bit")
    print(f"Kinesis directory: {os.path.abspath(args.kinesis_dir)}")
    print(
        f"PDXC2TriggerParams: size={ctypes.sizeof(PDXC2TriggerParams)}, "
        f"alignment={ctypes.alignment(PDXC2TriggerParams)}"
    )

    with Controller(
        args.serial,
        kinesis_dir=args.kinesis_dir,
        trigger_request_settling_s=args.request_delay,
    ) as controller:
        print(f"Connected to PDXC2 serial {controller.serial}")
        print(f"Device software version (raw): 0x{controller.get_software_version_raw():08X}")
        before_mode = controller.get_trigger_mode()
        before_params = controller.get_trigger_params()
        print(f"Before mode: {_format_mode(before_mode)}")
        print(f"Before params: {before_params.as_dict()}")

        if args.write_analog is not None:
            print("Writing with trigger mode forced to Manual first...")
            actual = controller.configure_analog_rising(
                *args.write_analog,
                arm=args.arm_analog_rising,
                persist=args.persist,
            )
            after_mode = controller.get_trigger_mode()
            after_params = controller.get_trigger_params()
            print(f"Verified params after write: {actual.as_dict()}")
            print(f"After mode: {_format_mode(after_mode)}")
            print(f"After params: {after_params.as_dict()}")
            if args.persist:
                print("PDXC2_PersistSettings returned success.")
        else:
            print("Read-only diagnostic complete; no settings were changed.")

    print("Controller closed cleanly. Kinesis GUI may now be opened for a separate check.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
