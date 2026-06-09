"""
Continuously prints the voltage of every enabled channel 
(except trigger channel)

Used for quick position calibration to find the center (maximum intensity) of the beam

Updates every 0.25seconds
"""

import ctypes
import time
import numpy as np

from pico.pico_config import PICO_CONFIG

from pico.setup import(
    open_scope,
    get_max_adc,
    close_scope,
    config_channels
)

from pico.timing import(
    get_sample_regions,
    find_timebase
)

from pico.trigger import disable_trigger

from picosdk.functions import adc2mV, assert_pico_ok
from picosdk.ps2000a import ps2000a as ps

REFRESH_PERIOD_S = 0.05

def main():
    chandle = open_scope()

    try:
        max_adc = get_max_adc(chandle)
        channels = PICO_CONFIG["channels"]
        channel_ranges = PICO_CONFIG["channel_ranges"]

        config_channels(chandle, channels, channel_ranges)

        disable_trigger(chandle)

        pre_trigger_samples = 0
        post_trigger_samples = 1000
        total_samples = post_trigger_samples

        timebase = find_timebase(chandle, total_samples, desired_sample_interval_ns=1000)

        while True:
            buffers = {}
            for name, channel in channels.items():
                if name == PICO_CONFIG["trigger_channel_name"]:
                    continue
                buffer = (ctypes.c_int16 * total_samples)()

                status = ps.ps2000aSetDataBuffer(
                    chandle,
                    channel,
                    buffer,
                    total_samples,
                    0,
                    ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]
                )
                assert_pico_ok(status)

                buffers[name] = buffer
            
            time_unavail_ms = ctypes.c_int32()

            status = ps.ps2000aRunBlock(
                chandle,
                pre_trigger_samples,
                post_trigger_samples,
                timebase,
                0,
                ctypes.byref(time_unavail_ms),
                0,
                None,
                None
            )
            assert_pico_ok(status)

            ready = ctypes.c_int16(0)

            while ready.value == 0:
                status = ps.ps2000aIsReady(chandle, ctypes.byref(ready))
                assert_pico_ok(status)
            
            samples_returned = ctypes.c_int32(total_samples)
            overflow = ctypes.c_int16()

            status = ps.ps2000aGetValues(
                chandle,
                0,
                ctypes.byref(samples_returned),
                1,
                ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"],
                0,
                ctypes.byref(overflow)
            )
            assert_pico_ok(status)

            output = []

            for name, buffer in buffers.items():
                data_mV = adc2mV(buffer, channel_ranges[name], max_adc)
                voltage_mV = float(np.mean(data_mV))
                output.append(f"{name}: {voltage_mV:.2f} mV")
            
            print(" | ".join(output))

            time.sleep(REFRESH_PERIOD_S)

    except KeyboardInterrupt:
        print("\nStopped by user.")
    
    finally:
        close_scope(chandle)

if __name__ == "__main__":
    main()