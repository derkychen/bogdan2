import ctypes

def assert_pico_ok(status: int) -> None: ...
def mV2adc(
    mv: float, range_id: int, max_adc: ctypes.c_int16
) -> ctypes.c_int16: ...
def adc2mV(
    buffer: ctypes.Array[ctypes.c_int16],
    range_id: int,
    max_adc: ctypes.c_int16,
) -> list[float]: ...
