"""Constants and configurations for importing."""

import ctypes
from dataclasses import dataclass


@dataclass
class KinesisConfig:
    """Constants used the usage of Thorlabs Kinesis."""

    KINESIS_DIR: str = r"C:\Program Files\Thorlabs\Kinesis"
    DLL_FILE: str = "Thorlabs.MotionControl.Benchtop.Piezo.dll"

    # Values that represent Kinesis PDXC2 Trigger Modes
    PDXC2_TRIGGER_MODE_MANUAL = 0x00
    PDXC2_TRIGGER_MODE_ANALOG_RISING = 0x01
    PDXC2_TRIGGER_MODE_ANALOG_FALLING = 0x02
    PDXC2_TRIGGER_MODE_FIXED_STEP_RISING = 0x03
    PDXC2_TRIGGER_MODE_FIXED_STEP_FALLING = 0x04
    PDXC2_TRIGGER_MODE_TWO_POSITION_RISING = 0x05
    PDXC2_TRIGGER_MODE_TWO_POSITION_FALLING = 0x06


@dataclass
class PDXC2Config:
    """Constants and configurations pertaining to each controller."""

    SERIAL_NUM: bytes

    ANALOG_IN_GAIN: ctypes.c_float
    ANALOG_IN_OFFSET: ctypes.c_float

    ANALOG_OUT_GAIN: ctypes.c_float
    ANALOG_OUT_OFFSET: ctypes.c_float


@dataclass
class XPDXC2Config(PDXC2Config):
    """Configurations pertaining to the x-axis controller."""

    PDXC2_SERIAL_NUM: bytes = b"112547939"

    ANALOG_IN_GAIN: ctypes.c_float = ctypes.c_float(3.0303)
    ANALOG_IN_OFFSET: ctypes.c_float = ctypes.c_float(0.0)

    # TODO: Calibrate these values, if controller output is stable, increase
    # gain to 3.3
    ANALOG_OUT_GAIN: ctypes.c_float = ctypes.c_float(3.0)
    ANALOG_OUT_OFFSET: ctypes.c_float = ctypes.c_float(0.0)


@dataclass
class YPDXC2Config(PDXC2Config):
    """Configurations pertaining to the y-axis controller."""

    PDXC2_SERIAL_NUM: bytes = b"112512664"

    ANALOG_IN_GAIN: ctypes.c_float = ctypes.c_float(3.0303)
    ANALOG_IN_OFFSET: ctypes.c_float = ctypes.c_float(0.0)

    # TODO: Calibrate these values, if controller output is stable, increase
    # gain to 3.3
    ANALOG_OUT_GAIN: ctypes.c_float = ctypes.c_float(3.0)
    ANALOG_OUT_OFFSET: ctypes.c_float = ctypes.c_float(0.0)
