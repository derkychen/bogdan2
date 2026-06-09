import ctypes
import time
import numpy as np

from picosdk.functions import adc2mV, assert_pico_ok
from picosdk.ps2000a import ps2000a as ps

from pico.setup import load_settings, open_scope, get_max_adc, close_scope, config_channels
from pico.timing import get_sample_regions, find_timebase
from pico.trigger import set_simple_trigger, calculate_trigger_threshold_mV
from pico.beam_point import make_beam_point, average_beam_points

""" Block mode stores captures in PicoScope's segment memory before transferring to memory allocated in PC"""

class PicoBlockMode:
    def __init__(self, config):
        self.config = config
        self.settings = load_settings(config["settings_json_path"])

        self.chandle = None
        self.max_adc = None

        self.channels = config["channels"]
        self.channel_ranges = config["channel_ranges"]

        self.pre_trigger_samples = None
        self.post_trigger_samples = None
        self.total_samples = None

        self.timebase = None
        self.num_pulses = self.settings["num_pulses"]
        self.oversample = self.config["oversample"]

        self.trigger_channel_name = config["trigger_channel_name"]
        self.trigger_threshold_mV = None
        self.trigger_direction = config["trigger_direction"]

    def setup(self):
        self.chandle = open_scope()
        self.max_adc = get_max_adc(self.chandle)
        config_channels(self.chandle, self.channels, self.channel_ranges)

        (
            self.pre_trigger_samples, 
            self.post_trigger_samples, 
            self.total_samples
        ) = get_sample_regions(
            self.settings["pre_trigger_time_ns"],
            self.settings["post_trigger_time_ns"],
            self.settings["sample_interval_ns"]
        )
        
        print("pre_trigger_time_ns:", self.settings["pre_trigger_time_ns"])
        print("post_trigger_time_ns:", self.settings["post_trigger_time_ns"])
        print("sample_interval_ns:", self.settings["sample_interval_ns"])

        print("Number of pre-trigger samples:", self.pre_trigger_samples)
        print("Number of post-trigger samples:", self.post_trigger_samples)
        print(self.num_pulses, "pulses per position")

        if self.total_samples <= 0:
            raise ValueError(
                "total_samples is 0. Check pre_trigger_time_ns, "
                "post_trigger_time_ns, and sample_interval_ns in your JSON file."
        )

        print(self.num_pulses, "pulses per position")

        self.timebase = find_timebase(self.chandle, self.total_samples, self.settings["sample_interval_ns"])
        
        trigger_channel = self.channels[self.trigger_channel_name]
        trigger_range = self.channel_ranges[self.trigger_channel_name]

        
        print(self.config)

        if self.config["trigger_threshold_mode"] == "auto":
           self.trigger_threshold_mV = calculate_trigger_threshold_mV(
               self.chandle,
               trigger_channel,
               trigger_range,
               self.max_adc,
               self.timebase,
               self.oversample,
               sigma_multiplier=self.config["sigma_multiplier"],
               baseline_samples=1000
           )
        elif self.config["trigger_threshold_mode"] == "manual":
           self.trigger_threshold_mV = self.config["manual_trigger_threshold_mV"]
           print("Using manual trigger threshold:", self.trigger_threshold_mV, "mV")
        else:
            raise ValueError(
               "trigger_threshold_mode must be either 'auto' or 'manual'"
           )


        set_simple_trigger(
            self.chandle,
            trigger_channel,
            trigger_range,
            self.max_adc,
            self.trigger_threshold_mV,
            self.trigger_direction
        )

    def create_rapid_block_buffers(self):
        """
        Prepares storage locations (memory buffers) for block captures.
        Creates one memory segment per pulse and one buffer per channel per segment.
        """

        max_samples = ctypes.c_int32()
        status = ps.ps2000aMemorySegments(self.chandle, self.num_pulses, ctypes.byref(max_samples))     #create one memory segment per pulse
        assert_pico_ok(status)

        #print("Max samples per segment:", max_samples.value)
        if self.total_samples > max_samples.value:
            raise ValueError(
                f"total_samples={self.total_samples} is larger than"
                f"max_samples_per_segment={max_samples.value}"
            )

        status = ps.ps2000aSetNoOfCaptures(self.chandle, self.num_pulses)
        assert_pico_ok(status)

        buffers = {}

        for name, channel in self.channels.items():
            buffers[name] = []                                              #create empty list for each channel with one memory buffer for each capture segment

            for segment_index in range(self.num_pulses):
                buffer = (ctypes.c_int16 * self.total_samples)()
                buffers[name].append(buffer)                                #store buffer inside dictionary

                status = ps.ps2000aSetDataBuffer(                           #store captured waveform into buffer
                    self.chandle,
                    channel,
                    buffer,                                   #memory address of buffer array
                    self.total_samples,
                    segment_index,
                    ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]
                )
                assert_pico_ok(status)
        
        return buffers

    def run_rapid_block_capture(self, timeout_s=None):
        """ 
        Waits for laser pulses then capturs waveforms
        """

        if timeout_s is None:
            timeout_s = self.config["timeout_s"]
        
        time_unavail_ms = ctypes.c_int32()

        status = ps.ps2000aRunBlock(                                    #run block capture; starts waiting for trigger
            self.chandle,
            self.pre_trigger_samples,
            self.post_trigger_samples,
            self.timebase,
            self.oversample,
            ctypes.byref(time_unavail_ms),
            0,
            None,
            None
        )
        assert_pico_ok(status)

        print("Waiting for trigger...")

        start = time.time()
        ready = ctypes.c_int16(0)

        while ready.value == 0:
            status = ps.ps2000aIsReady(self.chandle, ctypes.byref(ready))        #continuously checking if captures are complete
            assert_pico_ok(status)

            elapsed = time.time() - start

            if elapsed > timeout_s:
                raise TimeoutError(f"Picoscope did not trigger within {timeout_s} seconds.")
           
            time.sleep(0.001)
        
        print("PicoScope triggered.")
        print("Captures stored in PicoScope memory.")

        samples_returned = ctypes.c_uint32(self.total_samples)
        overflow = (ctypes.c_int16 * self.num_pulses)()

        '''
        #DEBUG
        print("num_pulses:", self.num_pulses)
        print("from segment:", 0)
        print("to segment:", self.num_pulses - 1)
        print("samples requested:", samples_returned.value)
        '''

        #transfer captures from pico memory to PC
        status = ps.ps2000aGetValuesBulk(                               #transfer captures to python buffer
            self.chandle,
            ctypes.byref(samples_returned),
            0,
            self.num_pulses - 1,
            0,
            ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"],
            overflow
        )
        assert_pico_ok(status)

        if any(overflow):                                                           #error check if outside signal range
            print("WARNING: ADC overflow detected in one or more captures.")

        print("Captures transferred to computer.")
        print("Samples returned per capture:", samples_returned.value)

        return samples_returned

    def convert_capture_to_mV(self,buffers, capture_index):
        """
        Converts ADC buffers from one capture segment to mV
        """

        data_mV = {}

        for name in self.channels.keys():
            data_mV[name] = adc2mV(
                buffers[name][capture_index],
                self.channel_ranges[name],
                self.max_adc
            )
        
        return data_mV
    
    def process_rapid_block_data(self, buffers):
        """
        Converts each pulse to a beam point, then averages
        """

        beam_points = []

        intensity_measurements = []
        x_positions = []
        y_positions = []
        
        for capture_index in range(self.num_pulses):
            data_mV = self.convert_capture_to_mV(buffers, capture_index)
            
            beam_point = make_beam_point(
                data_mV,
                self.pre_trigger_samples,
                intensity_channel_name = "intensity",
                x_channel_name = "x_position",
                y_channel_name = "y_position",

            )
        
            print(
                f"Pulse {capture_index + 1} peak:",
                beam_point["peak_intensity)mV"],
                "mV"
            )

            beam_points.append(beam_point)

        averaged_point = average_beam_points(beam_points)
        return averaged_point
        
    def capture_beam_point(self):
        buffers = self.create_rapid_block_buffers()
        self.run_rapid_block_capture()
        beam_point = self.process_rapid_block_data(buffers)

        return beam_point

    def close(self):
        if self.chandle is not None:
            close_scope(self.chandle)
            self.chandle = None
