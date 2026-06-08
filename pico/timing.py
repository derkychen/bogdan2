import ctypes
from picosdk.ps2000a import ps2000a as ps

def get_sample_regions(pre_trigger_time_ns, post_trigger_time_ns, sample_interval_ns):
    pre_trigger_samples = int(pre_trigger_time_ns / sample_interval_ns)
    post_trigger_samples = int(post_trigger_time_ns / sample_interval_ns)

    total_samples = (pre_trigger_samples + post_trigger_samples)

    return (pre_trigger_samples, post_trigger_samples, total_samples)

def find_timebase(chandle, total_samples, desired_sample_interval_ns):
    """
    Finds the first PicoScope timebase whose sample interval is greater than or equal to the requested interval
    """

    timebase = 1        #very fast 
    
    while True:
        dt_ns = ctypes.c_float()
        returned_max_samples = ctypes.c_int32()

        status = ps.ps2000aGetTimebase2(
            chandle,
            timebase,
            total_samples,
            ctypes.byref(dt_ns),
            0,                                          #oversample
            ctypes.byref(returned_max_samples),
            0                                           #segment index
        )

        if status == 0:
            if dt_ns.value >= desired_sample_interval_ns:
                print(
                "Timebase:", timebase,
                "| Sample Interval: ", dt_ns.value, "ns"
                )
            
                return timebase
        
        timebase += 1