from typing import ClassVar, Self

from System import Enum

class DeviceSettings: ...

class DeviceConfiguration:
    class DeviceSettingsUseOptionType(Enum):
        UseDeviceSettings: ClassVar[Self]
        UseFileSettings: ClassVar[Self]
        UseConfiguredSettings: ClassVar[Self]

class DeviceManagerCLI:
    @staticmethod
    def BuildDeviceList() -> None: ...
