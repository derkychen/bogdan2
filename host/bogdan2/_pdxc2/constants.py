"""Constants and configurations for importing."""

from typing import Final

TRIGGER_MODE_ANALOG_RISING: Final[int] = 0x01
TRIGGER_MODE_MANUAL: Final[int] = 0x00

CONTROL_MODE_CLOSED_LOOP: Final[int] = 0x02

ANALOG_IN_GAIN: Final[float] = 1.0
ANALOG_IN_OFFSET_MV: Final[float] = -10000.0

ANALOG_OUT_GAIN: Final[float] = 1.0
ANALOG_OUT_OFFSET_MV: Final[float] = 0.0
