"""Utilities for setting up the PDXC2s.

By default, both controllers are in Manual Trigger Mode. The utilities provided
in this module are mainly to set the Trigger Modes of both controllers to
Analog Rising Edge. This module does not provide complete functionality, only
what is necessary.
"""

import ctypes
import os
import time
from typing import Final

KINESIS_DIR: Final[str] = r"C:\Program Files\Thorlabs\Kinesis"
KINESIS_DLL_FILE: Final[str] = "Thorlabs.MotionControl.Benchtop.Piezo.dll"

ENABLE_SETTLING_TIME_S: Final[float] = 0.500
TRIGGER_MODE_ANALOG_RISING: Final[int] = 0x01


class KinesisStatusFailure(Exception):
    """When a Thorlabs C library function fails."""


def _check_err_status_code(func, *args) -> None:
    """For Thorlabs C library functions that return status codes."""
    ret = func(*args)

    if ret != 0:
        raise KinesisStatusFailure(
            f"{func.__name__} failed with status code {ret}."
        )


def _check_err_bool(func, *args) -> None:
    """For Thorlabs C library functions that return Booleans."""
    ret = func(*args)

    if not ret:
        raise KinesisStatusFailure(f"{func.__name__} failed.")


class PDXC2TriggerParams(ctypes.Structure):
    """Mirrors the PDXC2_TriggerParams structure from the Kinesis C API."""

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


class Controller:
    """A wrapper around the Thorlabs Kinesis C Library.

    Provides methods to enable and configure the PDXC2.
    """

    def _set_function_prototypes(self) -> None:
        """Set argument and return types for the library calls used."""
        self._lib.TLI_BuildDeviceList.argtypes = []
        self._lib.TLI_BuildDeviceList.restype = ctypes.c_short

        self._lib.PDXC2_Open.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_Open.restype = ctypes.c_short

        self._lib.PDXC2_Close.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_Close.restype = None

        self._lib.PDXC2_StartPolling.argtypes = [
            ctypes.c_char_p,
            ctypes.c_int,
        ]
        self._lib.PDXC2_StartPolling.restype = ctypes.c_bool

        self._lib.PDXC2_StopPolling.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_StopPolling.restype = None

        self._lib.PDXC2_Enable.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_Enable.restype = ctypes.c_short

        self._lib.PDXC2_RequestExternalTriggerConfig.argtypes = [
            ctypes.c_char_p
        ]
        self._lib.PDXC2_RequestExternalTriggerConfig.restype = ctypes.c_short

        self._lib.PDXC2_GetExternalTriggerConfig.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_GetExternalTriggerConfig.restype = ctypes.c_uint16

        self._lib.PDXC2_SetExternalTriggerConfig.argtypes = [
            ctypes.c_char_p,
            ctypes.c_uint16,
        ]
        self._lib.PDXC2_SetExternalTriggerConfig.restype = ctypes.c_short

        self._lib.PDXC2_RequestExternalTriggerParams.argtypes = [
            ctypes.c_char_p
        ]
        self._lib.PDXC2_RequestExternalTriggerParams.restype = ctypes.c_short

        self._lib.PDXC2_GetExternalTriggerParams.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(PDXC2TriggerParams),
        ]
        self._lib.PDXC2_GetExternalTriggerParams.restype = ctypes.c_short

        self._lib.PDXC2_SetExternalTriggerParams.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(PDXC2TriggerParams),
        ]
        self._lib.PDXC2_SetExternalTriggerParams.restype = ctypes.c_short

        self._lib.PDXC2_PersistSettings.argtypes = [ctypes.c_char_p]
        self._lib.PDXC2_PersistSettings.restype = ctypes.c_bool

    def __init__(self, serial_num: bytes) -> None:
        """Initialize a `PDXC2` object.

        Args:
            serial_num: The serial number in bytes form of the PDXC2, found
                        on the back. It is prefixed with 'SN:'.
        """
        self._serial_num = ctypes.c_char_p(serial_num)

        self._lib = ctypes.cdll.LoadLibrary(
            os.path.join(KINESIS_DIR, KINESIS_DLL_FILE)
        )

        self._set_function_prototypes()

    def enable(self) -> None:
        """Enable the controller."""
        _check_err_status_code(self._lib.TLI_BuildDeviceList)
        _check_err_status_code(self._lib.PDXC2_Open, self._serial_num)
        _check_err_bool(self._lib.PDXC2_StartPolling, self._serial_num, 200)

        # Settling delay as specified by the Kinesis API.
        time.sleep(ENABLE_SETTLING_TIME_S)
        _check_err_status_code(self._lib.PDXC2_Enable, self._serial_num)
        time.sleep(ENABLE_SETTLING_TIME_S)

        print(f"PDXC2 ({self._serial_num.value}) enabled.")

    def get_trigger_mode(self) -> int:
        """Get the Trigger Mode."""
        _check_err_status_code(
            self._lib.PDXC2_RequestExternalTriggerConfig, self._serial_num
        )

        return self._lib.PDXC2_GetExternalTriggerConfig(self._serial_num)

    def get_trigger_params(self) -> PDXC2TriggerParams:
        """Get trigger parameters."""
        _check_err_status_code(
            self._lib.PDXC2_RequestExternalTriggerParams, self._serial_num
        )

        params = PDXC2TriggerParams()

        _check_err_status_code(
            self._lib.PDXC2_GetExternalTriggerParams,
            self._serial_num,
            ctypes.byref(params),
        )

        return params

    def set_to_analog_rising_trigger_mode(self) -> None:
        """Set Trigger Mode to Analog Rising Edge."""
        _check_err_status_code(
            self._lib.PDXC2_SetExternalTriggerConfig,
            self._serial_num,
            TRIGGER_MODE_ANALOG_RISING,
        )

    def set_analog_rising_trigger_params(
        self,
        analog_in_gain: float,
        analog_in_offset: float,
        analog_out_gain: float,
        analog_out_offset: float,
    ) -> None:
        """Set trigger parameters for the Analog Rising Trigger Mode."""
        params = self.get_trigger_params()

        params.AnalogInGain = float(analog_in_gain)
        params.AnalogInOffset = float(analog_in_offset)
        params.AnalogOutGain = float(analog_out_gain)
        params.AnalogOutOffset = float(analog_out_offset)

        _check_err_status_code(
            self._lib.PDXC2_SetExternalTriggerParams,
            self._serial_num,
            ctypes.byref(params),
        )

    def persist_settings(self) -> None:
        """Persist settings to the controller.

        This allows the settings to be used even after the controller is
        disconnected. However, the controller's settings will reset (in
        particular, Trigger Mode will be reset to Manual) when it is switched
        off or disconnected from power.
        """
        _check_err_bool(self._lib.PDXC2_PersistSettings, self._serial_num)

    # TODO: More docstring information on exactly what this function does.
    def close(self) -> None:
        """Close the device."""
        self._lib.PDXC2_StopPolling(self._serial_num)
        self._lib.PDXC2_Close(self._serial_num)
