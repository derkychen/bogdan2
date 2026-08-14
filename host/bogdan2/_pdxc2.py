"""Utilities for setting up the PDXC2s.




By default, both controllers are left in Manual Trigger Mode. The utilities
provided here configure the PDXC2 for closed-loop analog-triggered operation.




This implementation uses only the Thorlabs Kinesis .NET API through Python.NET.
"""

from __future__ import annotations


import time
from enum import IntEnum
from pathlib import Path
from typing import Final


from pythonnet import load


load("netfx")


import clr  # noqa: E402


KINESIS_DIR: Final[Path] = Path(r"C:\Program Files\Thorlabs\Kinesis")
SETTINGS_INITIALIZATION_TIMEOUT_MS: Final[int] = 10_000
POLLING_INTERVAL_MS: Final[int] = 250
SETTLING_TIME_S: Final[float] = 0.25


# Exact closed-loop values from the previously working PDXC2 configuration.
PROPORTIONAL: Final[int] = 8192
INTEGRAL: Final[int] = 8192
DIFFERENTIAL: Final[int] = 0


for _assembly in (
    "Thorlabs.MotionControl.DeviceManagerCLI.dll",
    "Thorlabs.MotionControl.GenericPiezoCLI.dll",
    "Thorlabs.MotionControl.Benchtop.PiezoCLI.dll",
):
    clr.AddReference(str(KINESIS_DIR / _assembly))


from System import Convert, Enum  # noqa: E402
from Thorlabs.MotionControl.Benchtop.PiezoCLI.PDXC2 import (  # noqa: E402
    InertiaStageController,
    PDXC2Settings,
)
from Thorlabs.MotionControl.DeviceManagerCLI import (  # noqa: E402
    DeviceConfiguration,
    DeviceManagerCLI,
)
from Thorlabs.MotionControl.GenericPiezoCLI.Piezo import (  # noqa: E402
    PiezoControlModeTypes,
)


class ControlModeID(IntEnum):
    CLOSED_LOOP = 2
    CLOSED_LOOP_SMOOTH = 4


class TriggerModeID(IntEnum):
    MANUAL = 0
    ANALOG_RISING = 1


class Controller:
    """A small wrapper around the Thorlabs Kinesis .NET API."""

    def __init__(self, serial_num: str) -> None:
        """Initialize a `PDXC2` object."""
        self._serial_num = serial_num
        self._device = None
        self._polling = False

    def _require_device(self):
        device = self._device
        if device is None:
            raise RuntimeError("PDXC2 is not open.")
        return device

    def _set_trigger_mode(self, trigger_mode_id: TriggerModeID) -> None:
        """Set the external-trigger mode."""
        device = self._require_device()

        # The managed trigger enum type is obtained from the controller itself.
        # This avoids inventing an enum class name that is not used in the
        # published PDXC2 Python example.
        current_mode = device.GetExternalTriggerConfig()
        requested_mode = Enum.ToObject(
            current_mode.GetType(),
            int(trigger_mode_id),
        )
        device.SetExternalTriggerConfig(requested_mode)

    def open(self) -> None:
        """Acquire controller hardware."""
        if self._device is not None:
            return

        DeviceManagerCLI.BuildDeviceList()

        device = InertiaStageController.CreateInertiaStageController(self._serial_num)
        if device is None:
            raise RuntimeError(f"Could not create PDXC2 {self._serial_num}.")

        self._device = device

        try:
            device.Connect(self._serial_num)
            time.sleep(0.25)

            device.StartPolling(250)
            self._polling = True
            time.sleep(0.25)

            device.EnableDevice()
            time.sleep(0.25)

            if not device.IsSettingsInitialized():
                device.WaitForSettingsInitialized(10_000)

            if not device.IsSettingsInitialized():
                raise RuntimeError(
                    f"PDXC2 {self._serial_num} settings failed to initialize."
                )

            configuration = device.GetPDXC2Configuration(
                self._serial_num,
                DeviceConfiguration.DeviceSettingsUseOptionType.UseFileSettings,
            )

            settings = PDXC2Settings.GetSettings(configuration)

            if settings is None:
                raise RuntimeError("PDXC2Settings.GetSettings returned null.")

            # This is the exact transaction established by Test C.
            device.SetSettings(settings, True, True)
            time.sleep(0.5)

        except BaseException:
            self.close()
            raise

    def ensure_control_mode(
        self,
        control_mode_id: ControlModeID,
    ) -> None:
        """Set the requested closed-loop control mode."""
        device = self._require_device()

        match control_mode_id:
            case ControlModeID.CLOSED_LOOP:
                mode = PiezoControlModeTypes.CloseLoop
            case ControlModeID.CLOSED_LOOP_SMOOTH:
                mode = PiezoControlModeTypes.CloseLoopSmooth
            case _:
                raise ValueError(f"Unsupported control mode: {control_mode_id!r}.")

        # Thorlabs' PDXC2 examples set the mode directly; they do not require a
        # managed GetPositionControlMode readback.
        device.SetPositionControlMode(mode)

    def ensure_closedloop_params(
        self,
        refspeed: int,
        acceleration: int,
    ) -> None:
        """Set the closed-loop motion parameters used by the profiler."""
        if not 0 <= refspeed <= 0xFFFFFFFF:
            raise ValueError("refspeed must fit uint32.")
        if not 0 <= acceleration <= 0xFFFFFFFF:
            raise ValueError("acceleration must fit uint32.")

        device = self._require_device()
        params = device.GetClosedLoopParameters()

        # Reproduce the values from the previously working ctypes controller,
        # rather than inheriting PID values from the Kinesis file profile.
        params.RefSpeed = refspeed
        params.Proportional = PROPORTIONAL
        params.Integral = INTEGRAL
        params.Differential = DIFFERENTIAL
        params.Acceleration = acceleration

        device.SetClosedLoopParameters(params)

    def ensure_analog_rising_trigger_params(
        self,
        in_gain: float,
        in_offset: float,
        out_gain: float,
        out_offset: float,
    ) -> None:
        device = self._require_device()

        self.ensure_trigger_mode(TriggerModeID.MANUAL)

        params = device.GetExternalTriggerParameters()

        params.AnalogInGain = Convert.ToDecimal(in_gain)
        params.AnalogInOffset = Convert.ToDecimal(in_offset)
        params.AnalogOutGain = Convert.ToDecimal(out_gain)
        params.AnalogOutOffset = Convert.ToDecimal(out_offset)

        device.SetExternalTriggerParameters(params)
        time.sleep(0.25)

        actual = device.GetExternalTriggerParameters()

        print(
            self._serial_num,
            "trigger params:",
            actual.AnalogInGain,
            actual.AnalogInOffset,
            actual.AnalogOutGain,
            actual.AnalogOutOffset,
        )

    def ensure_trigger_mode(
        self,
        trigger_mode_id: TriggerModeID,
    ) -> None:
        device = self._require_device()

        current = device.GetExternalTriggerConfig()

        requested = Enum.ToObject(
            current.GetType(),
            int(trigger_mode_id),
        )

        device.SetExternalTriggerConfig(requested)
        time.sleep(0.25)

        actual = device.GetExternalTriggerConfig()

        if int(actual) != int(trigger_mode_id):
            raise RuntimeError(
                "Failed to set trigger mode: "
                f"requested={int(trigger_mode_id)}, "
                f"actual={int(actual)}"
            )

    def close(self) -> None:
        """Close the device."""
        device = self._device
        if device is None:
            return

        try:
            if self._polling:
                device.StopPolling()
        finally:
            self._polling = False
            try:
                device.Disconnect(True)
            finally:
                self._device = None
