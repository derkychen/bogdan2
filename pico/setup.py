import ctypes
import json
from pathlib import Path
from picosdk.functions import  assert_pico_ok
from picosdk.ps2000a import ps2000a as ps

def load_settings(settings_json_path):
    path = Path(settings_json_path)

    if not path.is_absolute():
        project_root = Path(__file__).resolve().parents[1]
        path = project_root / path
    
    with open(path, "r", encoding="utf-8") as f:
        settings = json.load(f)
    
    required_keys = [
        "pre_trigger_time_ns",
        "post_trigger_time_ns",
        "sample_interval_ns",
        "num_pulses",
    ]

    missing = [key for key in required_keys if key not in settings]

    if missing:
        raise KeyError(f"Missing required JSON setting(s): {missing}")
    return settings

def open_scope():
    chandle = ctypes.c_int16()
    status = ps.ps2000aOpenUnit(ctypes.byref(chandle), None)

    assert_pico_ok(status)

    print("Opened PicoScope. Handle:", chandle.value)

    return chandle

def get_max_adc(chandle):
    max_adc = ctypes.c_int16()
    status = ps.ps2000aMaximumValue(
        chandle,
        ctypes.byref(max_adc)
    )

    assert_pico_ok(status)

    return max_adc

def config_channels(chandle, channels, channel_ranges):
    """ 
    Enables each channel in the channels dictionary 
    """

    for name, channel in channels.items():
        status = ps.ps2000aSetChannel(
            chandle,
            channel,
            1,
            ps.PS2000A_COUPLING["PS2000A_DC"],
            channel_ranges[name],
            0.0                                     #no offset
        )

        assert_pico_ok(status)

    return channels

def close_scope(chandle):
    if chandle is None:
        return
    
    status = ps.ps2000aStop(chandle)
    assert_pico_ok(status)

    status = ps.ps2000aCloseUnit(chandle)
    assert_pico_ok(status)

    print("Closed PicoScope successfully.")