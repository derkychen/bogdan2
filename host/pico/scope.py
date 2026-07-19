"""Abstraction of the PicoScope."""

import ctypes
import time

import numpy as np
from pico.channel import Channel
from pico.config import (
    ChannelRanges,
    Channels,
    RatioMode,
    ScopeMode,
    TriggerDirection,
)
from picosdk.functions import assert_pico_ok
from picosdk.ps2000a import ps2000a as ps


class Scope:
    """Abstraction of the PicoScope."""

    def _set_max_adc(self) -> None:
        """Set the internal maximum ADC value."""
        assert_pico_ok(
            ps.ps2000aMaximumValue(self._chandle, ctypes.byref(self._max_adc))
        )

    def _set_timebase(self, timebase: int) -> None:
        """Set the timebase of the PicoScope."""
        self._timebase = timebase

    def __init__(
        self,
    ) -> None:
        """Initialize the PicoScope."""
        self._chandle = ctypes.c_int16()
        self._max_adc = ctypes.c_int16()
        self._timebase = 1

        self.mode = None
        self._pretrigger_samples = None
        self._posttrigger_samples = None
        self._total_samples = None

        self._a = None
        self._b = None
        self._c = None
        self._d = None

        self._channels = [self._a, self._b, self._c, self._d]

    def get_chandle(self) -> ctypes.c_int16:
        """Get the C handle of the PicoScope."""
        return self._chandle

    def get_max_adc(self) -> ctypes.c_int16:
        """Get the C handle of the PicoScope."""
        return self._max_adc

    def get_timebase(self) -> int:
        """Get the C handle of the PicoScope."""
        return self._timebase

    def open(self) -> None:
        """Open the PicoScope and set the internal C handle."""
        assert_pico_ok(ps.ps2000aOpenUnit(ctypes.byref(self._chandle), None))

    def setup(
        self,
        a_range_id: int = ChannelRanges.V5,
        b_range_id: int = ChannelRanges.V20,
        c_range_id: int = ChannelRanges.V20,
        d_range_id: int = ChannelRanges.V20,
    ) -> None:
        """Set up the PicoScope."""
        self._set_max_adc()
        self._set_timebase()

    def set_channel_ranges(
        self,
        a_range_id: int = ChannelRanges.V5,
        b_range_id: int = ChannelRanges.V20,
        c_range_id: int = ChannelRanges.V20,
        d_range_id: int = ChannelRanges.V20,
    ) -> None:
        """Set the PicoScope channel ranges."""
        self._a = Channel(self, Channels.A, a_range_id)
        self._b = Channel(self, Channels.B, b_range_id)
        self._c = Channel(self, Channels.C, c_range_id)
        self._d = Channel(self, Channels.D, d_range_id)

    def set_mode(self, mode: int) -> None:
        """Set the mode of the PicoScope.

        Whether the PicoScope is triggered depends on whether it is capturing
        for calibration or profiling.
        """
        assert mode == ScopeMode.SINGLE or mode == ScopeMode.BULK, (
            "The scope cannot be in any other mode."
        )

        self._mode = mode

        if self._mode == ScopeMode.SINGLE:
            self._a.disable_trigger()
        else:
            self._a.set_trigger(TriggerDirection.RISING, threshold_mv=2000.0)

    def set_sample_region(
        self,
        pretrigger_time_ns: int,
        posttrigger_time_ns: int,
        sample_interval_ns: int = 4,
    ) -> None:
        """Set the sample region of the PicoScope.

        Also sets the corresponding timebase, which is the first PicoScope
        timebase whose sample interval is greater than or equal to the
        requested interval.
        """
        self._pretrigger_samples = int(pretrigger_time_ns / sample_interval_ns)
        self._posttrigger_samples = int(
            posttrigger_time_ns / sample_interval_ns
        )
        self.total_samples = self._total_samples

        self._timebase = 1

        while True:
            dt_ns = ctypes.c_float()
            returned_max_samples = ctypes.c_int32()

            assert_pico_ok(
                ps.ps2000aGetTimebase2(
                    self._chandle,
                    self._timebase,
                    self._total_samples,
                    ctypes.byref(dt_ns),
                    0,
                    ctypes.byref(returned_max_samples),
                    0,
                )
            )

            if dt_ns.value >= sample_interval_ns:
                print(
                    "Timebase:",
                    self._timebase,
                    "| Sample Interval: ",
                    dt_ns.value,
                    "ns",
                )

                return

            self._timebase += 1

        self._set_timebase(
            self._total_samples,
            sample_interval_ns,
        )

    def create_single_buffers(self) -> None:
        """Create buffers for a single capture.

        Each channel has one buffer.

        NOTE: This function should only be called when in `bulk` mode.
        """
        for channel in self._channels:
            buffer = (ctypes.c_int16 * (self._total_samples))()

            assert_pico_ok(
                ps.ps2000aSetDataBuffers(
                    self._chandle,
                    channel.get_id(),
                    buffer,
                    None,
                    self._total_samples,
                    0,
                    RatioMode.NONE,
                )
            )

    def create_bulk_buffers(self, num_acquisition_buffers: int) -> None:
        """Create buffers for a bulk capture.

        Each channel has one buffer. Each channel's buffer contains a number of
        buffers equal to the number of pulses to be captured.

        NOTE: This function should only be called when in `bulk` mode.
        """
        max_samples = ctypes.c_int32()

        assert_pico_ok(
            ps.ps2000aMemorySegments(
                self._chandle,
                self.num_acquisition_buffers,
                ctypes.byref(max_samples),
            )
        )

        if self._total_samples > max_samples.value:
            raise ValueError(
                f"Total number of samples ({self.total_samples}) is larger"
                f"than the maximum number of samples per acquisition buffer"
                f"{max_samples.value}."
            )

        status = ps.ps2000aSetNoOfCaptures(self.chandle, self.num_pulses)
        assert_pico_ok(status)

        for channel in self._channels:
            channel_buffer = []
            channel.set_buffer(channel_buffer)

            for i in range(num_acquisition_buffers):
                acquisition_buffer = (ctypes.c_int16 * (self._total_samples))()

                channel.channel_buffer_add_acquisition_buffer(
                    acquisition_buffer
                )

                assert_pico_ok(
                    ps.ps2000aSetDataBuffer(
                        self._chandle,
                        channel.get_id(),
                        acquisition_buffer,
                        self._total_samples,
                        i,
                        RatioMode.NONE,
                    )
                )

    def run_block_capture(self, timeout_s: float = 10.0) -> None:
        """Capture waveforms on every trigger."""
        unavailable_ms = ctypes.c_int32()

        assert_pico_ok(
            ps.ps2000aRunBlock(
                self.chandle,
                self._pretrigger_samples,
                self._posttrigger_samples,
                self.timebase,
                self.oversample,
                ctypes.byref(unavailable_ms),
                0,
                None,
                None,
            )
        )

        start = time.time()
        ready = ctypes.c_int16(0)

        while ready.value == 0:
            assert_pico_ok(
                ps.ps2000aIsReady(self.chandle, ctypes.byref(ready))
            )

            elapsed = time.time() - start

            if elapsed > timeout_s:
                raise TimeoutError(
                    f"Picoscope did not capture within {timeout_s} seconds."
                )

            time.sleep(0.001)

    def get_single(self) -> dict[str, float]:
        """Receive single capture from the PicoScope in millivolts."""
        samples = ctypes.c_int32(self._total_samples)
        overflow = ctypes.c_int16()

        assert_pico_ok(
            ps.ps2000aGetValues(
                self._chandle,
                0,
                ctypes.byref(samples),
                1,
                RatioMode.NONE,
                0,
                ctypes.byref(overflow),
            )
        )

        if overflow.value != 0:
            print("WARNING: ADC overflow detected in capture.")

        return {
            channel.name: channel.single_mv_from_buffer
            for channel in self._channels
        }

    def get_bulk(self) -> dict[str, np.array]:
        """Receive bulk capture from the PicoScope in millivolts."""
        samples = ctypes.c_uint32(self.total_samples)
        overflow = (ctypes.c_int16 * self.num_pulses)()

        # Transfer captures from PicoScope memory to host.
        assert_pico_ok(
            ps.ps2000aGetValuesBulk(
                self.chandle,
                ctypes.byref(samples),
                0,
                self.num_pulses - 1,
                0,
                RatioMode.NONE,
                overflow,
            )
        )

        # Check if any captures fell outside of signal range.
        if any(overflow):
            print("WARNING: ADC overflow detected in one or more captures.")

        return {
            channel.name: channel.bulk_mv_from_buffer
            for channel in self._channels
        }

    def close(self) -> None:
        """Close the PicoScope."""
        assert_pico_ok(ps.ps2000aStop(self._chandle))
        assert_pico_ok(ps.ps2000aCloseUnit(self._chandle))
