import ctypes
import time

from picosdk.functions import adc2mV, assert_pico_ok
from picosdk.ps2000a import ps2000a as ps

from pico.setup import (
    load_settings,
    open_scope,
    get_max_adc,
    config_channels,
    close_scope
)

from pico.timing import get_sample_regions, find_timebase
from pico.trigger import disable_trigger

def setup_no_trigger_block_capture(config):
    """
    Opens Picoscope, configures channels, disables trigger,
    calculates sample counts, and finds the timebase
    """

    settings = load_settings(config["settings_json_path"])

    chandle = open_scope()
    max_adc = get_max_adc(chandle)

    channels = config["channels"]
    channel_ranges = config["channel_ranges"]

    config_channels(chandle, channels, channel_ranges)

    disable_trigger(chandle)
    
    pre_trigger_samples, post_trigger_samples, total_samples = get_sample_regions(
        settings["pre_trigger_time_ns"],
        settings["post_trigger_time_ns"],
        settings["sample_interval_ns"]
    )

    if total_samples <= 0:
        raise ValueError("Total samples must be greater than 0. Check your pre and post trigger times and sampling interval.")

    timebase = find_timebase(chandle, total_samples, settings["sample_interval_ns"])

    print("No-trigger block capture setup complete.")

    return{
        "settings": settings,
        "chandle": chandle,
        "max_adc": max_adc,
        "channels": channels,
        "channel_ranges": channel_ranges,
        "pre_trigger_samples": pre_trigger_samples,
        "post_trigger_samples": post_trigger_samples,
        "total_samples": total_samples,
        "timebase": timebase,
        "oversample": config["oversample"]
    }

def capture_single_no_trigger_block(acq):
    """
    Takes one no-trigger block-mode capture and returns voltage data
    """
    channels = acq["channels"]
    channel_ranges = acq["channel_ranges"]

    #print("RUNNING FROM:", __file__)
    #print("channels used for buffers:", channels.keys())

    buffers = {}

    for channel_name, channel in channels.items():
        #print("creating buffer for:", channel_name)

        buffer = (ctypes.c_int16 * acq["total_samples"])()
        buffers[channel_name] = buffer
        status = ps.ps2000aSetDataBuffers(
            acq["chandle"],
            channel,
            buffer,
            None,
            acq["total_samples"],
            0,
            ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]
        )
        assert_pico_ok(status)
    
    #print("buffers created:", buffers.keys())

    time_unavail_ms = ctypes.c_int32()

    status = ps.ps2000aRunBlock(
        acq["chandle"],
        acq["pre_trigger_samples"],
        acq["post_trigger_samples"],
        acq["timebase"],
        acq["oversample"],
        ctypes.byref(time_unavail_ms),
        0,
        None,
        None
    )
    assert_pico_ok(status)

    ready = ctypes.c_int16(0)

    while ready.value == 0:
        status = ps.ps2000aIsReady(acq["chandle"], ctypes.byref(ready))
        assert_pico_ok(status)
        time.sleep(0.01)
    
    samples_returned = ctypes.c_int32(acq["total_samples"])
    overflow = ctypes.c_int16()

    status = ps.ps2000aGetValues(
        acq["chandle"],
        0,
        ctypes.byref(samples_returned),
        1,
        ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"],
        0,
        ctypes.byref(overflow)
    )
    assert_pico_ok(status)

    if overflow.value != 0:
        print("WARNING: ADC overflow detected.")
    
    data_mV = {}
    for channel_name in buffers.keys():
        data_mV[channel_name] = adc2mV(
            buffers[channel_name],
            channel_ranges[channel_name],
            acq["max_adc"]
        )
    return data_mV, samples_returned.value

def close_acquisition(acq):
    if acq is not None and acq.get("chandle") is not None:
        close_scope(acq["chandle"])
        acq["chandle"] = None
