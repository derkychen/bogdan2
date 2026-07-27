"""Constants and configurations for importing."""

from typing import Final

from picosdk.ps2000a import ps2000a as ps

RANGE_1V: Final[int] = ps.PS2000A_RANGE["PS2000A_1V"]
RANGE_2V: Final[int] = ps.PS2000A_RANGE["PS2000A_2V"]
RANGE_5V: Final[int] = ps.PS2000A_RANGE["PS2000A_5V"]
RANGE_10V: Final[int] = ps.PS2000A_RANGE["PS2000A_10V"]
RANGE_20V: Final[int] = ps.PS2000A_RANGE["PS2000A_20V"]
RANGE_50V: Final[int] = ps.PS2000A_RANGE["PS2000A_50V"]

TRIGGER_RISING: Final[int] = ps.PS2000A_THRESHOLD_DIRECTION["PS2000A_RISING"]

RATIO_MODE_NONE: Final[int] = ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]

SCOPE_MODE_SINGLE: Final[int] = 0
SCOPE_MODE_BULK: Final[int] = 1
