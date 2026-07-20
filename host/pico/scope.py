"""Abstraction of the PicoScope."""

import ctypes
import time
from typing import Final

import numpy as np
from picosdk.functions import assert_pico_ok
from picosdk.ps2000a import ps2000a as ps

from host.pico.channel import Channel
from host.pico.constants import (
    RATIO_MODE_NONE,
    SCOPE_MODE_BULK,
    SCOPE_MODE_SINGLE,
    TRIGGER_RISING,
)

CHANNEL_A: Final[int] = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_A"]
CHANNEL_B: Final[int] = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_B"]
CHANNEL_C: Final[int] = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_C"]
CHANNEL_D: Final[int] = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_D"]


class Scope:
    """Abstraction of the PicoScope."""

    def _set_max_adc(self) -> None:
        """Set the internal maximum ADC value."""
        assert_pico_ok(
            ps.ps2000aMaximumValue(self._chandle, ctypes.byref(self._max_adc))
        )

    def __init__(
        self,
    ) -> None:
        """Initialize the PicoScope."""
        self._chandle = ctypes.c_int16()
        self._max_adc = ctypes.c_int16()
        self._timebase = None

        self._mode = None

        self._pretrigger_samples = None
        self._posttrigger_samples = None
        self._total_samples = None

        self._a = None
        self._b = None
        self._c = None
        self._d = None

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
    ) -> None:
        """Set up the PicoScope."""
        self._set_max_adc()
        self._timebase = 1

    def configure_channels(
        self,
        a_name: str,
        a_range_id: int,
        b_name: str,
        b_range_id: int,
        c_name: str,
        c_range_id: int,
        d_name: str,
        d_range_id: int,
    ) -> None:
        """Configure PicoScope channel names and ranges."""
        self._a = Channel(self, a_name, CHANNEL_A, a_range_id)
        self._b = Channel(self, b_name, CHANNEL_B, b_range_id)
        self._c = Channel(self, c_name, CHANNEL_C, c_range_id)
        self._d = Channel(self, d_name, CHANNEL_D, d_range_id)

        self._channels = [self._a, self._b, self._c, self._d]

    def set_mode(self, mode: int) -> None:
        """Set the mode of the PicoScope.

        Whether the PicoScope is triggered depends on whether it is capturing
        for calibration or profiling.
        """
        assert mode in (SCOPE_MODE_SINGLE, SCOPE_MODE_BULK), (
            "The scope cannot be in any other mode."
        )

        self._mode = mode

        if self._mode == SCOPE_MODE_SINGLE:
            self._a.disable_trigger()
        else:
            self._a.set_trigger(TRIGGER_RISING, threshold_mv=2000.0)

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
        self._total_samples = (
            self._pretrigger_samples + self._posttrigger_samples
        )

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

                break

            self._timebase += 1

    def configure_single_capture(self) -> None:
        """Create buffers for a single capture.

        Each channel has one buffer.

        NOTE: This function should only be called when in `SINGLE` mode.
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
                    RATIO_MODE_NONE,
                )
            )

    def configure_bulk_capture(self, num_captures: int) -> None:
        """Create buffers for a bulk capture.

        Each channel has one buffer. Each channel's buffer contains a number of
        buffers equal to the number of pulses to be captured.

        NOTE: This function should only be called when in `BULK` mode.
        """
        max_samples = ctypes.c_int32()

        assert_pico_ok(
            ps.ps2000aMemorySegments(
                self._chandle,
                num_captures,
                ctypes.byref(max_samples),
            )
        )

        if self._total_samples > max_samples.value:
            raise ValueError(
                f"Total number of samples ({self.total_samples}) is larger"
                f"than the maximum number of samples per acquisition buffer"
                f"{max_samples.value}."
            )

        assert_pico_ok(ps.ps2000aSetNoOfCaptures(self._chandle, num_captures))

        for channel in self._channels:
            channel_buffer = []
            channel.set_buffer(channel_buffer)

            for i in range(num_captures):
                segment = (ctypes.c_int16 * (self._total_samples))()

                channel.channel_buffer_add_segment(segment)

                assert_pico_ok(
                    ps.ps2000aSetDataBuffer(
                        self._chandle,
                        channel.get_id(),
                        segment,
                        self._total_samples,
                        i,
                        RATIO_MODE_NONE,
                    )
                )

    def run_capture(self, timeout_s: float = 10.0) -> None:
        """Capture waveforms on every trigger."""
        unavailable_ms = ctypes.c_int32()

        assert_pico_ok(
            ps.ps2000aRunBlock(
                self._chandle,
                self._pretrigger_samples,
                self._posttrigger_samples,
                self._timebase,
                0,
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
                RATIO_MODE_NONE,
                0,
                ctypes.byref(overflow),
            )
        )

        if overflow.value != 0:
            print("WARNING: ADC overflow detected in capture.")

        return {
            channel.name: channel.single_mv_from_buffer()
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
                RATIO_MODE_NONE,
                overflow,
            )
        )

        # Check if any captures fell outside of signal range.
        if any(overflow):
            print("WARNING: ADC overflow detected in one or more captures.")

        return {
            channel.name: channel.bulk_mv_from_buffer()
            for channel in self._channels
        }

    def close(self) -> None:
        """Close the PicoScope."""
        assert_pico_ok(ps.ps2000aStop(self._chandle))
        assert_pico_ok(ps.ps2000aCloseUnit(self._chandle))
