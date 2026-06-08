import ctypes
import numpy as np

from picosdk.functions import adc2mV, assert_pico_ok, mV2adc
from picosdk.ps2000a import ps2000a as ps

def set_simple_trigger(
   chandle,
   trigger_channel,
   trigger_range,
   max_adc,
   threshold_mV,
   direction="PS2000A_RISING"
):
   
   """
   Sets a simple hardware trigger.
   """
   
   trigger_adc = mV2adc(threshold_mV, trigger_range, max_adc)

   print("Requested trigger threshold:", threshold_mV, "mV")
   print("Converted trigger threshold:", trigger_adc, "ADC")
   print("Trigger channel enum:", trigger_channel)
   print("Trigger range enum:", trigger_range)
   print("Max ADC:", max_adc.value)

   status = ps.ps2000aSetSimpleTrigger(
       chandle,
       1,
       trigger_channel,
       trigger_adc,
       ps.PS2000A_THRESHOLD_DIRECTION[direction],
       0,
       0
   )
   assert_pico_ok(status)

   print(f"Trigger configured at {threshold_mV:.3f} mV")

def disable_trigger(chandle):
   status = ps.ps2000aSetSimpleTrigger(
       chandle,
       0,
       0,
       0,
       0,
       0,
       0
   )
   assert_pico_ok(status)

   print("Trigger disabled.")

def calculate_trigger_threshold_mV(
   chandle,
   trigger_channel,
   trigger_range,
   max_adc,
   timebase,
   oversample,
   sigma_multiplier=7,
   baseline_samples=1000,
):
    """
    Calculates a trigger threshold from a baseline capture.
    threshold = baseline_mean + sigma_multiplier * baseline_noise
    """
    disable_trigger(chandle)

    buffer = (ctypes.c_int16 * baseline_samples)()

    status = ps.ps2000aSetDataBuffer(
        chandle,
        trigger_channel,
        buffer,
        baseline_samples,
        0,
        ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]
    )
    assert_pico_ok(status)

    time_unavail_ms = ctypes.c_int32()

    status = ps.ps2000aRunBlock(
        chandle,
        0,
        baseline_samples,
        timebase,
        oversample,
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

    samples_returned = ctypes.c_int32(baseline_samples)
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

    if overflow.value != 0:
        print("WARNING: ADC overflow detected during baseline threshold capture.")

    baseline_mV_array = np.array(
        adc2mV(buffer, trigger_range, max_adc),
        dtype=float
    )

    baseline_mean_mV = float(np.mean(baseline_mV_array))
    baseline_noise_mV = float(np.std(baseline_mV_array))

    threshold_mV = baseline_mean_mV + (sigma_multiplier * baseline_noise_mV)

    print("\nAutomatic trigger threshold calculation:")
    print(f"Baseline mean: {baseline_mean_mV:.3f} mV")
    print(f"Baseline noise RMS: {baseline_noise_mV:.3f} mV")
    print(f"Sigma multiplier: {sigma_multiplier}")
    print(f"Calculated trigger threshold: {threshold_mV:.3f} mV\n")

    return threshold_mV



