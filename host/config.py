"""Constants and configurations for importing."""

from dataclasses import dataclass
from typing import ClassVar


@dataclass
class KinesisConfig:
    """Constants used the usage of Thorlabs Kinesis."""

    KINESIS_DIR: ClassVar[str] = r"C:\Program Files\Thorlabs\Kinesis"
    DLL_FILE: ClassVar[str] = "Thorlabs.MotionControl.Benchtop.Piezo.dll"

    # Values that represent Kinesis PDXC2 Trigger Modes
    PDXC2_TRIGGER_MODE_MANUAL: ClassVar[int] = 0x00
    PDXC2_TRIGGER_MODE_ANALOG_RISING: ClassVar[int] = 0x01
    PDXC2_TRIGGER_MODE_ANALOG_FALLING: ClassVar[int] = 0x02
    PDXC2_TRIGGER_MODE_FIXED_STEP_RISING: ClassVar[int] = 0x03
    PDXC2_TRIGGER_MODE_FIXED_STEP_FALLING: ClassVar[int] = 0x04
    PDXC2_TRIGGER_MODE_TWO_POSITION_RISING: ClassVar[int] = 0x05
    PDXC2_TRIGGER_MODE_TWO_POSITION_FALLING: ClassVar[int] = 0x06


@dataclass
class PDXC2Config:
    """Constants and configurations pertaining to each controller."""

    serial_num: bytes

    analog_in_gain: float = 3.0303
    analog_in_offset: float = 0.0

    # TODO: Calibrate these values, if controller output is stable, increase
    # gain to 3.3
    analog_out_gain: float = 3.0
    analog_out_offset: float = 0.0


@dataclass
class XPDXC2Config(PDXC2Config):
    """Configurations pertaining to the x-axis controller."""

    serial_num: bytes = b"112547939"


@dataclass
class YPDXC2Config(PDXC2Config):
    """Configurations pertaining to the y-axis controller."""

    serial_num: bytes = b"112512664"
