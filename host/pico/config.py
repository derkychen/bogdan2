"""Constants and configurations for importing."""

from dataclasses import dataclass

from picosdk.ps2000a import ps2000a


@dataclass
class Channels:
    """Channel numbers."""

    A: int = ps2000a.PS2000A_CHANNEL["PS2000A_CHANNEL_A"]
    B: int = ps2000a.PS2000A_CHANNEL["PS2000A_CHANNEL_B"]
    C: int = ps2000a.PS2000A_CHANNEL["PS2000A_CHANNEL_C"]
    D: int = ps2000a.PS2000A_CHANNEL["PS2000A_CHANNEL_D"]


@dataclass
class ChannelRanges:
    """Channel ranges.

    NOTE: This is not the full set of ranges. It only contains the ranges used
          in this project.
    """

    V1: int = ps2000a.PS2000A_RANGE["PS2000A_1V"]
    V2: int = ps2000a.PS2000A_RANGE["PS2000A_2V"]
    V5: int = ps2000a.PS2000A_RANGE["PS2000A_5V"]
    V10: int = ps2000a.PS2000A_RANGE["PS2000A_10V"]
    V20: int = ps2000a.PS2000A_RANGE["PS2000A_20V"]
    V50: int = ps2000a.PS2000A_RANGE["PS2000A_50V"]


@dataclass
class ChannelCoupling:
    """Channel ranges.

    NOTE: This is not the full set of couplings. It only contains the couplings
          used in this project.
    """

    DC: int = ps2000a.PS2000A_COUPLING["PS2000A_DC"]


@dataclass
class TriggerDirections:
    """Wrap trigger direction.

    NOTE: This is not the full set of threshold directions. It only contains
          the threshold directions used in this project.
    """

    RISING: int = ps2000a.PS2000A_THRESHOLD_DIRECTION["PS2000A_RISING"]


@dataclass
class RatioMode:
    """Channel ratio mode.

    NOTE: This is not the full set of ratio modes. It only contains the ratio
          modes used in this project.
    """

    NONE: int = ps2000a.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]


@dataclass
class ScopeMode:
    """Scope mode, different for calibration and profiling.

    The main difference is that the PicoScope is not configured to trigger on
    the rising edge in `SINGLE` mode, which is useful during calibration.
    """

    SINGLE: int = 0
    BULK: int = 1
