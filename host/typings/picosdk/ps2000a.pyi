from collections.abc import Callable, Mapping

class Ps2000alib:
    PS2000A_CHANNEL: Mapping[str, int]
    PS2000A_COUPLING: Mapping[str, int]
    PS2000A_RANGE: Mapping[str, int]
    PS2000A_RATIO_MODE: Mapping[str, int]
    PS2000A_THRESHOLD_DIRECTION: Mapping[str, int]

    ps2000aOpenUnit: Callable[..., int]
    ps2000aCloseUnit: Callable[..., int]
    ps2000aIsReady: Callable[..., int]
    ps2000aGetTimebase2: Callable[..., int]
    ps2000aGetValues: Callable[..., int]
    ps2000aGetValuesBulk: Callable[..., int]
    ps2000aSetChannel: Callable[..., int]
    ps2000aSetNoOfCaptures: Callable[..., int]
    ps2000aSetSimpleTrigger: Callable[..., int]
    ps2000aSetDataBuffer: Callable[..., int]
    ps2000aSetDataBuffers: Callable[..., int]
    ps2000aMaximumValue: Callable[..., int]
    ps2000aRunBlock: Callable[..., int]
    ps2000aMemorySegments: Callable[..., int]
    ps2000aStop: Callable[..., int]

ps2000a: Ps2000alib
