from typing import ClassVar, Self, overload

from System import Decimal, Enum
from Thorlabs.MotionControl.DeviceManagerCLI import (
    DeviceConfiguration,
    DeviceSettings,
)
from Thorlabs.MotionControl.GenericPiezoCLI.Piezo import (
    PiezoControlModeTypes,
)

class TriggerModes(Enum):
    Manual: ClassVar[Self]
    AnalogRising: ClassVar[Self]
    AnalogFalling: ClassVar[Self]
    FixedStepRising: ClassVar[Self]
    FixedStepFalling: ClassVar[Self]
    TwoPositionRising: ClassVar[Self]
    TwoPositionFalling: ClassVar[Self]

class ClosedLoopParams:
    Acceleration: int
    Differential: int
    Integral: int
    Proportional: int
    RefSpeed: int

class TriggerParameters:
    AnalogInGain: Decimal
    AnalogInOffset: Decimal
    AnalogOutGain: Decimal
    AnalogOutOffset: Decimal

class PDXC2Configuration(DeviceConfiguration): ...

class PDXC2Settings(DeviceSettings):
    @staticmethod
    def GetSettings(
        deviceConfiguration: DeviceConfiguration,
    ) -> PDXC2Settings | None: ...

class InertiaStageController:
    @staticmethod
    def CreateInertiaStageController(
        serialNumber: str,
    ) -> InertiaStageController | None: ...
    def Connect(
        self,
        serialNumber: str,
    ) -> None: ...
    def EnableDevice(self) -> None: ...
    def StartPolling(
        self,
        ms: int,
    ) -> None: ...
    def StopPolling(self) -> None: ...
    def IsSettingsInitialized(self) -> bool: ...
    def WaitForSettingsInitialized(
        self,
        timeout: int,
    ) -> None: ...
    def GetPDXC2Configuration(
        self,
        serialNumber: str,
        startupSettingsMode: DeviceConfiguration.DeviceSettingsUseOptionType,
    ) -> PDXC2Configuration: ...
    @overload
    def SetSettings(
        self,
        deviceSettings: DeviceSettings,
        persist: bool,
    ) -> None: ...
    @overload
    def SetSettings(
        self,
        deviceSettings: DeviceSettings,
        updateDevice: bool,
        persist: bool,
    ) -> None: ...
    def GetPositionControlMode(
        self,
    ) -> PiezoControlModeTypes: ...
    def SetPositionControlMode(
        self,
        positionControlMode: PiezoControlModeTypes,
    ) -> None: ...
    def GetClosedLoopParameters(
        self,
    ) -> ClosedLoopParams: ...
    def SetClosedLoopParameters(
        self,
        closedLoopParameters: ClosedLoopParams,
    ) -> None: ...
    def GetExternalTriggerConfig(
        self,
    ) -> TriggerModes: ...
    def SetExternalTriggerConfig(
        self,
        mode: TriggerModes,
    ) -> None: ...
    def GetExternalTriggerParameters(
        self,
    ) -> TriggerParameters: ...
    def SetExternalTriggerParameters(
        self,
        parameters: TriggerParameters,
    ) -> None: ...
    def Disconnect(
        self,
        shutDown: bool,
    ) -> None: ...
