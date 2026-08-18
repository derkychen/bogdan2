"""Abstraction of the PicoScope."""

from __future__ import annotations

import ctypes as ct
import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Final

import numpy as np
import numpy.typing as npt
from picosdk.functions import adc2mV, assert_pico_ok, mV2adc
from picosdk.ps2000a import ps2000a as ps

from bogdan2._utils import ceil_div

TIMEBASE_MAX_TRIES: Final[int] = 10


class CouldNotFindTimebase(Exception):
    """When searching for a timebase according to sample region fails."""


class RangeID(IntEnum):
    V1 = ps.PS2000A_RANGE["PS2000A_1V"]
    V2 = ps.PS2000A_RANGE["PS2000A_2V"]
    V5 = ps.PS2000A_RANGE["PS2000A_5V"]
    V10 = ps.PS2000A_RANGE["PS2000A_10V"]
    V20 = ps.PS2000A_RANGE["PS2000A_20V"]
    V50 = ps.PS2000A_RANGE["PS2000A_50V"]


class TriggerDirectionID(IntEnum):
    RISING = ps.PS2000A_THRESHOLD_DIRECTION["PS2000A_RISING"]


class _CouplingID(IntEnum):
    DC = ps.PS2000A_COUPLING["PS2000A_DC"]


class _RatioModeID(IntEnum):
    NONE = ps.PS2000A_RATIO_MODE["PS2000A_RATIO_MODE_NONE"]


class _ChannelID(IntEnum):
    A = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_A"]
    B = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_B"]
    C = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_C"]
    D = ps.PS2000A_CHANNEL["PS2000A_CHANNEL_D"]


@dataclass(kw_only=True)
class ChannelParams:
    name: str
    range_id: RangeID


class Channel:
    """Abstraction for a channel of the PicoScope."""

    def __init__(
        self,
        chandle: ct.c_int16,
        max_adc: ct.c_int16,
        name: str,
        channel_id: _ChannelID,
        range_id: RangeID,
    ) -> None:
        """Initialize a PicoScope channel."""
        self._chandle: ct.c_int16 = chandle
        self._max_adc: ct.c_int16 = max_adc

        self.name: str = name

        self._channel_id: _ChannelID = channel_id
        self._range_id: RangeID = range_id

        self._single_buffer: ct.Array[ct.c_int16]
        self._bulk_buffers: list[ct.Array[ct.c_int16]]

        assert_pico_ok(
            ps.ps2000aSetChannel(
                self._chandle,
                self._channel_id,
                1,
                _CouplingID.DC,
                self._range_id,
                0.0,
            )
        )

    def get_id(self) -> int:
        """Get the channel s (from the C enumeration)."""
        return self._channel_id

    def set_trigger(
        self,
        direction_id: TriggerDirectionID,
        threshold_mv: float = 2000.0,
    ) -> None:
        """Configure a PicoScope channel as a logical trigger."""
        trigger_adc: ct.c_int16 = mV2adc(
            threshold_mv, self._range_id, self._max_adc
        )

        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(
                self._chandle,
                1,
                self._channel_id,
                trigger_adc,
                direction_id,
                0,
                0,
            )
        )

    def single_buffer_create(self, samples: int) -> None:
        """Allocate the channel buffer."""
        buffer = (ct.c_int16 * samples)()

        assert_pico_ok(
            ps.ps2000aSetDataBuffers(
                self._chandle,
                self._channel_id,
                buffer,
                None,
                samples,
                0,
                _RatioModeID.NONE,
            )
        )

        self._single_buffer = buffer

    def bulk_buffer_create(self, samples: int, captures: int) -> None:
        """Add an acquisition buffer segment to a channel buffer."""
        buffers = []
        self._bulk_buffers = buffers

        for i in range(captures):
            segment = (ct.c_int16 * (samples))()

            self._bulk_buffers.append(segment)

            assert_pico_ok(
                ps.ps2000aSetDataBuffer(
                    self._chandle,
                    self._channel_id,
                    segment,
                    samples,
                    i,
                    _RatioModeID.NONE,
                )
            )

    def disable_trigger(self) -> None:
        """Disable a Picoscope channel trigger."""
        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(self._chandle, 0, 0, 0, 0, 0, 0)
        )

    def single_mv(self) -> npt.NDArray[np.float64]:
        """Get a reading in millivolts from the channel buffer."""
        return np.asarray(
            adc2mV(
                self._single_buffer,
                self._range_id,
                self._max_adc,
            ),
            dtype=np.float64,
        )

    def bulk_mv(self) -> list[npt.NDArray[np.float64]]:
        """Get an array of readings in millivolts from the channel buffer."""
        return [
            np.asarray(
                adc2mV(
                    adc_sample,
                    self._range_id,
                    self._max_adc,
                ),
                dtype=np.float64,
            )
            for adc_sample in self._bulk_buffers
        ]


class Scope:
    """Abstraction of the PicoScope."""

    def __init__(self, serial_num: str) -> None:
        """Initialize the PicoScope."""
        self._serial_num: str = serial_num

        self._chandle: ct.c_int16 = ct.c_int16()
        self._max_adc: ct.c_int16 = ct.c_int16()
        self._timebase: int

        self._pretrigger_samples: int
        self._posttrigger_samples: int
        self._total_samples: int

        self._sample_interval_ns: int
        self._num_captures: int

        self._a: Channel
        self._b: Channel
        self._c: Channel
        self._d: Channel

        self._channels: dict[str, Channel]

    def get_pretrigger_ns(self) -> int:
        """Get the pre-trigger sample interval of the PicoScope."""
        return self._pretrigger_samples * self._sample_interval_ns

    def get_posttrigger_ns(self) -> int:
        """Get the post-trigger sample interval of the PicoScope."""
        return self._posttrigger_samples * self._sample_interval_ns

    def get_sample_interval_ns(self) -> int:
        """Get the sample interval of the PicoScope."""
        return self._sample_interval_ns

    def open(self) -> None:
        """Open the PicoScope and set the internal C handle."""
        assert_pico_ok(
            ps.ps2000aOpenUnit(
                ct.byref(self._chandle), self._serial_num.encode()
            )
        )

    def setup(
        self,
    ) -> None:
        """Set up the PicoScope."""
        assert_pico_ok(
            ps.ps2000aMaximumValue(self._chandle, ct.byref(self._max_adc))
        )

        self._timebase = 1

    def configure_channels(
        self,
        a_params: ChannelParams,
        b_params: ChannelParams,
        c_params: ChannelParams,
        d_params: ChannelParams,
    ) -> None:
        """Configure PicoScope channel names and ranges."""
        self._a = Channel(
            self._chandle,
            self._max_adc,
            a_params.name,
            _ChannelID.A,
            a_params.range_id,
        )
        self._b = Channel(
            self._chandle,
            self._max_adc,
            b_params.name,
            _ChannelID.B,
            b_params.range_id,
        )
        self._c = Channel(
            self._chandle,
            self._max_adc,
            c_params.name,
            _ChannelID.C,
            c_params.range_id,
        )
        self._d = Channel(
            self._chandle,
            self._max_adc,
            d_params.name,
            _ChannelID.D,
            d_params.range_id,
        )

        self._channels = {
            a_params.name: self._a,
            b_params.name: self._b,
            c_params.name: self._c,
            d_params.name: self._d,
        }

    def set_sample_region(
        self,
        pretrigger_time_ns: int,
        posttrigger_time_ns: int,
        sample_interval_ns: int = 4,
    ) -> None:
        """Set the PicoScope sample region and corresponding timebase.

        The selected timebase is the first whose sample interval is greater
        than or equal to the requested interval. Sample counts are calculated
        from the selected timebase.
        """
        temp_pretrigger_samples = ceil_div(
            pretrigger_time_ns, sample_interval_ns
        )
        temp_posttrigger_samples = ceil_div(
            posttrigger_time_ns, sample_interval_ns
        )
        temp_total_samples = temp_pretrigger_samples + temp_posttrigger_samples

        for timebase in range(1, TIMEBASE_MAX_TRIES):
            dt_ns = ct.c_float()
            returned_max_samples = ct.c_int32()

            # NOTE: `assert_pico_ok` is not called here as it always reports an
            #       error. This behaviour is could be a bug.
            _ = ps.ps2000aGetTimebase2(
                self._chandle,
                timebase,
                temp_total_samples,
                ct.byref(dt_ns),
                0,
                ct.byref(returned_max_samples),
                0,
            )

            self._sample_interval_ns = int(dt_ns.value)

            # Recompute sample region with the selected timebase
            if self._sample_interval_ns >= sample_interval_ns:
                self._timebase = timebase

                self._pretrigger_samples = ceil_div(
                    pretrigger_time_ns, self._sample_interval_ns
                )
                self._posttrigger_samples = ceil_div(
                    posttrigger_time_ns, self._sample_interval_ns
                )
                self._total_samples = (
                    self._pretrigger_samples + self._posttrigger_samples
                )

                return

        raise CouldNotFindTimebase(
            f"Could not find a timebase for a requested {sample_interval_ns} "
            + "ns sample interval."
        )

    def disable_trigger_a(self) -> None:
        """Disable triggering of the PicoScope."""
        assert self._a is not None, "Channel not initialized."

        self._a.disable_trigger()

    def enable_trigger_a(
        self, direction_id: TriggerDirectionID, threshold_mv: float = 2000.0
    ) -> None:
        """Enable triggering of the PicoScope."""
        assert self._a is not None, "Channel not initialized."

        self._a.set_trigger(direction_id, threshold_mv=threshold_mv)

    def configure_single_capture(self) -> None:
        """Create buffers for a single capture.

        Each channel has one buffer.
        """
        self._num_captures = 1

        for _, channel in self._channels.items():
            channel.single_buffer_create(self._total_samples)

    def configure_bulk_capture(self, num_captures: int) -> None:
        """Create buffers for a bulk capture.

        Each channel has one buffer. Each channel's buffer contains a number of
        buffers equal to the number of pulses to be captured.
        """
        self._num_captures = num_captures
        max_samples = ct.c_int32()

        assert_pico_ok(
            ps.ps2000aMemorySegments(
                self._chandle,
                self._num_captures,
                ct.byref(max_samples),
            )
        )

        if self._total_samples > max_samples.value:
            raise ValueError(
                f"Total number of samples ({self._total_samples}) is larger "
                + "than the maximum number of samples per acquisition buffer "
                + f"{max_samples.value}."
            )

        assert_pico_ok(
            ps.ps2000aSetNoOfCaptures(self._chandle, self._num_captures)
        )

        for _, channel in self._channels.items():
            channel.bulk_buffer_create(self._total_samples, self._num_captures)

    def run_capture(self, timeout_s: float = 10.0) -> None:
        """Capture waveforms on every trigger."""
        unavailable_ms = ct.c_int32()

        assert_pico_ok(
            ps.ps2000aRunBlock(
                self._chandle,
                self._pretrigger_samples,
                self._posttrigger_samples,
                self._timebase,
                0,
                ct.byref(unavailable_ms),
                0,
                None,
                None,
            )
        )

        start = time.time()
        ready = ct.c_int16(0)

        while ready.value == 0:
            assert_pico_ok(ps.ps2000aIsReady(self._chandle, ct.byref(ready)))

            elapsed = time.time() - start

            if elapsed > timeout_s:
                raise TimeoutError(
                    f"Picoscope did not capture within {timeout_s} seconds."
                )

            time.sleep(0.001)

    def transfer_single_values(self) -> None:
        """Transfer single capture from PicoScope memory to host."""
        samples_u32 = ct.c_uint32(self._total_samples)
        overflow = ct.c_int16()

        assert_pico_ok(
            ps.ps2000aGetValues(
                self._chandle,
                0,
                ct.byref(samples_u32),
                1,
                _RatioModeID.NONE,
                0,
                ct.byref(overflow),
            )
        )

        # Check if the capture fell outside of signal range.
        if overflow.value != 0:
            print("WARNING: ADC overflow detected in capture.")

    def transfer_bulk_values(self) -> None:
        """Transfer bulk capture from PicoScope memory to host."""
        samples_u32 = ct.c_uint32(self._total_samples)
        overflow = (ct.c_int16 * self._num_captures)()

        assert_pico_ok(
            ps.ps2000aGetValuesBulk(
                self._chandle,
                ct.byref(samples_u32),
                0,
                self._num_captures - 1,
                0,
                _RatioModeID.NONE,
                overflow,
            )
        )

        # Check if any captures fell outside of signal range.
        if any(overflow):
            print("WARNING: ADC overflow detected in one or more captures.")

    def channel_single_mv(self, channel_name: str) -> npt.NDArray[np.float64]:
        """Return single capture from a PicoScope channel in millivolts."""
        return self._channels[channel_name].single_mv()

    def channel_bulk_mv(
        self, channel_name: str
    ) -> list[npt.NDArray[np.float64]]:
        """Return bulk capture from a PicoScope channel in millivolts."""
        return self._channels[channel_name].bulk_mv()

    def close(self) -> None:
        """Close the PicoScope."""
        assert_pico_ok(ps.ps2000aStop(self._chandle))
        assert_pico_ok(ps.ps2000aCloseUnit(self._chandle))
