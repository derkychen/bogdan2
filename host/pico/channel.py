"""Abstraction for a PicoScope channel."""

import ctypes
from typing import Final

import numpy as np
from picosdk.functions import adc2mV, assert_pico_ok, mV2adc
from picosdk.ps2000a import ps2000a as ps

from host.pico.constants import RATIO_MODE_NONE
from host.pico.scope import Scope

COUPLING_DC: Final[int] = ps.PS2000A_COUPLING["PS2000A_DC"]


class Channel:
    """Abstraction for a channel of the PicoScope."""

    def _calculate_trigger_threshold_mv(
        self,
        multiplier,
        samples=1000,
    ):
        """Calculate trigger threshold from a baseline capture.

        threshold_mv = baseline_mean_mv + multiplier * baseline_noise_mv
        """
        self.disable_trigger()

        buffer = (ctypes.c_int16 * samples)()

        assert_pico_ok(
            ps.ps2000aSetDataBuffer(
                self._scope_chandle,
                self._channel_id,
                buffer,
                samples,
                0,
                RATIO_MODE_NONE,
            )
        )

        time_unavail_ms = ctypes.c_int32()

        assert_pico_ok(
            ps.ps2000aRunBlock(
                self._scope_chandle,
                0,
                samples,
                self._scope_timebase,
                0,
                ctypes.byref(time_unavail_ms),
                0,
                None,
                None,
            )
        )

        ready = ctypes.c_int16(0)

        while ready.value == 0:
            assert_pico_ok(
                ps.ps2000aIsReady(self._scope_chandle, ctypes.byref(ready))
            )

        samples_returned = ctypes.c_int32(samples)
        overflow = ctypes.c_int16()

        assert_pico_ok(
            ps.ps2000aGetValues(
                self._scope_chandle,
                0,
                ctypes.byref(samples_returned),
                1,
                RATIO_MODE_NONE,
                0,
                ctypes.byref(overflow),
            )
        )

        if overflow.value != 0:
            print("WARNING: ADC overflow during baseline threshold capture.")

        baseline_mv_array = np.array(
            adc2mV(buffer, self._range_id, self._scope_max_adc), dtype=float
        )

        baseline_mean_mv = float(np.mean(baseline_mv_array))
        baseline_noise_mv = float(np.std(baseline_mv_array))

        threshold_mv = baseline_mean_mv + (multiplier * baseline_noise_mv)

        return threshold_mv

    def __init__(
        self,
        scope: Scope,
        name: str,
        channel_id: int,
        range_id: int,
    ) -> None:
        """Initialize a PicoScope channel.

        Args:
            scope: The PicoScope instance the channel is of.
            name: The name given to the channel.
            channel_id: Channel from `ctypes` enumeration.
            range_id: Channel range from `ctypes` enumeration.
        """
        self._scope_chandle = scope.get_chandle()
        self._scope_max_adc = scope.get_max_adc()
        self._scope_timebase = scope.get_timebase()

        self.name = name

        self._channel_id = channel_id
        self._range_id = range_id

        self._buffer = None
        self._readings = None

        assert_pico_ok(
            ps.ps2000aSetChannel(
                self._scope_chandle,
                self._channel_id,
                1,
                COUPLING_DC,  # NOTE: Only DC is used.
                self._range_id,
                0.0,
            )
        )

    def get_id(self) -> int:
        """Get the channel ID (from the C enumeration)."""
        return self._channel_id

    def set_buffer(self, buffer: list) -> None:
        """Set the channel buffer."""
        self._buffer = buffer

    def set_trigger(
        self,
        direction_id: int,
        threshold_mv: float | None = None,
        threshold_multiplier: int | None = None,
    ) -> None:
        """Configure a PicoScope channel as a logical trigger.

        Args:
            direction_id: Trigger direction from `ctypes` enumeration.
            threshold_mv: Trigger threshold. Default is `None`, which means the
                          threshold is calculated.
            threshold_multiplier: Trigger threshold multiplier. Used in the
                                  calculation of the threshold in millivolts if
                                  it is not provided.
        """
        if threshold_mv is None:
            assert threshold_multiplier is not None, (
                "Threshold multiplier must be provided if threshold is not."
            )

            threshold_mv = self._calculate_trigger_threshold_mv(
                threshold_multiplier
            )

        trigger_adc = mV2adc(threshold_mv, self._range_id, self._scope_max_adc)

        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(
                self._scope_chandle,
                1,
                self._channel_id,
                trigger_adc,
                direction_id,
                0,
                0,
            )
        )

    def disable_trigger(self):
        """Disable a Picoscope channel trigger."""
        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(self._scope_chandle, 0, 0, 0, 0, 0, 0)
        )

    def channel_buffer_add_segment(self, segment: ctypes.Array) -> None:
        """Add a acquisition buffer to a channel buffer."""
        self._buffer.append(segment)

    def single_mv_from_buffer(self) -> np.array:
        """Get a reading in millivolts from the channel buffer."""
        return adc2mV(
            self._buffer,
            self._range_id,
            self._scope_max_adc,
        )

    def bulk_mv_from_buffer(self) -> np.array:
        """Get an array of readings in millivolts from the channel buffer."""
        return np.array(
            [
                adc2mV(
                    adc_sample,
                    self._range_id,
                    self._scope_max_adc,
                )
                for adc_sample in self._buffer
            ]
        )
