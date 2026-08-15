# pyright: strict, reportMissingModuleSource=false

"""Utilities for setting up the PDXC2s.

Factory settings leave both controllers in Manual Trigger Mode. The utilities
here ensure before profiling that the controllers are in the proper modes in
order to run a beam profile.

This implementation uses the Thorlabs Kinesis .NET API through Python.NET.
Testing resulted in the conclusion that `ctypes` API is not able to reliably
configure the controllers. Specifically, it lacks the functionality provided by
`SetSettings`.
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

# TODO: Remove, tighten, or replace these delays with polling loops.
POLLING_INTERVAL_MS: Final[int] = 250
SETTINGS_INITIALIZATION_TIMEOUT_MS: Final[int] = 10_000
SETTINGS_SETTLING_TIME_S: Final[float] = 0.5
COMMAND_SETTLING_TIME_S: Final[float] = 0.25

PROPORTIONAL: Final[int] = 8192
INTEGRAL: Final[int] = 8192
DIFFERENTIAL: Final[int] = 0

UINT32_MAX: Final[int] = 0xFFFFFFFF

for _assembly in (
    "Thorlabs.MotionControl.DeviceManagerCLI.dll",
    "Thorlabs.MotionControl.GenericPiezoCLI.dll",
    "Thorlabs.MotionControl.Benchtop.PiezoCLI.dll",
):
    _ = clr.AddReference(str(KINESIS_DIR / _assembly))

# ruff: disable[E402]

from System import (
    Convert,
)
from Thorlabs.MotionControl.Benchtop.PiezoCLI.PDXC2 import (
    InertiaStageController,
    PDXC2Settings,
    TriggerModes,
)
from Thorlabs.MotionControl.DeviceManagerCLI import (
    DeviceConfiguration,
    DeviceManagerCLI,
)
from Thorlabs.MotionControl.GenericPiezoCLI.Piezo import (
    PiezoControlModeTypes,
)

# ruff: enable[E402]


class PDXC2ConfigurationFailure(Exception):
    """When configuring of the PDXC2 fails."""


class ControlModeID(IntEnum):
    OPEN_LOOP = 1
    CLOSED_LOOP = 2


class TriggerModeID(IntEnum):
    MANUAL = 0
    ANALOG_RISING = 1


class Controller:
    """Wrapper for the Thorlabs Kinesis .NET API."""

    def __init__(self, serial_num: str) -> None:
        """Initialize a `PDXC2` object."""
        self._device: InertiaStageController
        self._serial_num: str = serial_num
        self._polling: bool = False

    def open(self) -> None:
        """Acquire controller hardware."""
        DeviceManagerCLI.BuildDeviceList()

        device = InertiaStageController.CreateInertiaStageController(
            self._serial_num
        )

        if device is None:
            raise PDXC2ConfigurationFailure(
                f"Could not open PDXC2 (SN:{self._serial_num})."
            )

        self._device = device

        try:
            self._device.Connect(self._serial_num)
            time.sleep(COMMAND_SETTLING_TIME_S)

            self._device.StartPolling(POLLING_INTERVAL_MS)
            self._polling = True
            time.sleep(COMMAND_SETTLING_TIME_S)

            self._device.EnableDevice()
            time.sleep(COMMAND_SETTLING_TIME_S)

            if not self._device.IsSettingsInitialized():
                self._device.WaitForSettingsInitialized(
                    SETTINGS_INITIALIZATION_TIMEOUT_MS
                )

            if not self._device.IsSettingsInitialized():
                raise PDXC2ConfigurationFailure(
                    f"PDXC2 (SN:{self._serial_num}) settings initialization "
                    + "failed."
                )

            configuration = self._device.GetPDXC2Configuration(
                self._serial_num,
                DeviceConfiguration.DeviceSettingsUseOptionType.UseFileSettings,
            )

            settings = PDXC2Settings.GetSettings(configuration)

            if settings is None:
                raise PDXC2ConfigurationFailure(
                    "PDXC2Settings.GetSettings returned null."
                )

            # NOTE: This seems to be equivalent to some part in the Kinesis GUI
            #       startup + connection to controllers which enabled the
            #       controllers to truly be configured.
            self._device.SetSettings(settings, True, True)
            time.sleep(SETTINGS_SETTLING_TIME_S)

        except Exception as err:
            self.close()

            raise PDXC2ConfigurationFailure(
                f"Could not open PDXC2 (SN:{self._serial_num})"
            ) from err

        print(f"PDXC2 (SN:{self._serial_num}) opened successfully.")

    def ensure_closedloop_params(
        self,
        refspeed: int,
        acceleration: int,
    ) -> None:
        """Get and (if necessary) set closed-loop motion parameters."""
        if not 0 <= refspeed <= UINT32_MAX:
            raise ValueError("refspeed must fit `uint32`.")
        if not 0 <= acceleration <= UINT32_MAX:
            raise ValueError("acceleration must fit `uint32`.")

        params = self._device.GetClosedLoopParameters()

        if (
            params.RefSpeed != refspeed
            or params.Proportional != PROPORTIONAL
            or params.Integral != INTEGRAL
            or params.Differential != DIFFERENTIAL
            or params.Acceleration != acceleration
        ):
            params.RefSpeed = refspeed
            params.Proportional = PROPORTIONAL
            params.Integral = INTEGRAL
            params.Differential = DIFFERENTIAL
            params.Acceleration = acceleration

            self._device.SetClosedLoopParameters(params)
            time.sleep(COMMAND_SETTLING_TIME_S)

            actual = self._device.GetClosedLoopParameters()

            if (
                actual.RefSpeed != refspeed
                or actual.Proportional != PROPORTIONAL
                or actual.Integral != INTEGRAL
                or actual.Differential != DIFFERENTIAL
                or actual.Acceleration != acceleration
            ):
                raise PDXC2ConfigurationFailure(
                    f"Failed to set PDXC2 (SN:{self._serial_num}) "
                    + "closed-loop parameters."
                )

        print(
            f"Ensured PDXC2 (SN:{self._serial_num}) closed loop "
            + "parameters.\n"
            + f"\t`refspeed`:     {refspeed}\n"
            + f"\t`acceleration`: {acceleration}\n"
        )

    def ensure_control_mode(
        self,
        control_mode_id: ControlModeID,
    ) -> None:
        """Get and (if necessary) set the control mode."""
        match control_mode_id:
            case ControlModeID.OPEN_LOOP:
                requested = PiezoControlModeTypes.OpenLoop
            case ControlModeID.CLOSED_LOOP:
                requested = PiezoControlModeTypes.CloseLoop

        current = self._device.GetPositionControlMode()

        if int(current) != int(control_mode_id):
            self._device.SetPositionControlMode(requested)
            time.sleep(COMMAND_SETTLING_TIME_S)

            actual = self._device.GetPositionControlMode()

            if int(actual) != int(control_mode_id):
                raise PDXC2ConfigurationFailure(
                    f"Failed to set PDXC2 (SN:{self._serial_num}) "
                    + f"control mode to {control_mode_id}."
                )

        print(
            f"Ensured PDXC2 (SN:{self._serial_num}) control mode "
            + f"{control_mode_id}."
        )

    def ensure_analog_rising_trigger_params(
        self,
        in_gain: float,
        in_offset: float,
        out_gain: float,
        out_offset: float,
    ) -> None:
        """Get and (if necessary) set Analog Rising parameters."""
        self.ensure_trigger_mode(TriggerModeID.MANUAL)

        params = self._device.GetExternalTriggerParameters()

        in_gain_decimal = Convert.ToDecimal(in_gain)
        in_offset_decimal = Convert.ToDecimal(in_offset)
        out_gain_decimal = Convert.ToDecimal(out_gain)
        out_offset_decimal = Convert.ToDecimal(out_offset)

        if (
            params.AnalogInGain != in_gain_decimal
            or params.AnalogInOffset != in_offset_decimal
            or params.AnalogOutGain != out_gain_decimal
            or params.AnalogOutOffset != out_offset_decimal
        ):
            params.AnalogInGain = in_gain_decimal
            params.AnalogInOffset = in_offset_decimal
            params.AnalogOutGain = out_gain_decimal
            params.AnalogOutOffset = out_offset_decimal

            self._device.SetExternalTriggerParameters(params)
            time.sleep(COMMAND_SETTLING_TIME_S)

            actual = self._device.GetExternalTriggerParameters()

            if (
                actual.AnalogInGain != in_gain_decimal
                or actual.AnalogInOffset != in_offset_decimal
                or actual.AnalogOutGain != out_gain_decimal
                or actual.AnalogOutOffset != out_offset_decimal
            ):
                raise PDXC2ConfigurationFailure(
                    f"Failed to set PDXC2 (SN:{self._serial_num}) "
                    + "Analog Rising parameters."
                )

        print(
            f"Ensured PDXC2 (SN:{self._serial_num}) Analog Rising trigger "
            + "parameters.\n"
            + f"\t`Analog IN gain`:    {in_gain}\n"
            + f"\t`Analog IN offset`:  {in_offset}\n"
            + f"\t`Analog OUT gain`:   {out_gain}\n"
            + f"\t`Analog OUT offset`: {out_offset}\n"
        )

    def ensure_trigger_mode(
        self,
        trigger_mode_id: TriggerModeID,
    ) -> None:
        """Get and (if necessary) set Trigger Mode."""
        match trigger_mode_id:
            case TriggerModeID.MANUAL:
                requested = TriggerModes.Manual
            case TriggerModeID.ANALOG_RISING:
                requested = TriggerModes.AnalogRising

        current = self._device.GetExternalTriggerConfig()

        if int(current) != int(trigger_mode_id):
            self._device.SetExternalTriggerConfig(requested)
            time.sleep(COMMAND_SETTLING_TIME_S)

            actual = self._device.GetExternalTriggerConfig()

            if int(actual) != int(trigger_mode_id):
                raise PDXC2ConfigurationFailure("Failed to set trigger mode.")

        print(
            f"Ensured PDXC2 (SN:{self._serial_num}) trigger mode "
            + f"{trigger_mode_id}."
        )

    def close(self) -> None:
        """Close the device."""
        try:
            if self._polling:
                self._device.StopPolling()
        finally:
            self._polling = False
            self._device.Disconnect(True)
