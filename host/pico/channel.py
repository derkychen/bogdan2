"""Abstraction for a PicoScope channel."""

from __future__ import annotations

import ctypes
from typing import TYPE_CHECKING, Final

import numpy as np
from picosdk.functions import adc2mV, assert_pico_ok, mV2adc
from picosdk.ps2000a import ps2000a as ps

from host.pico.constants import RATIO_MODE_NONE

if TYPE_CHECKING:
    from host.pico.scope import Scope


COUPLING_DC: Final[int] = ps.PS2000A_COUPLING["PS2000A_DC"]


class Channel:
    """Abstraction for a channel of the PicoScope."""

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
        self._scope = scope

        self.name = name

        self._channel_id = channel_id
        self._range_id = range_id

        self._buffer = None
        self._readings = None

        assert_pico_ok(
            ps.ps2000aSetChannel(
                self._scope.get_chandle(),
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

    def set_trigger(
        self,
        direction_id: int,
        threshold_mv: float | None = None,
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
        trigger_adc = mV2adc(
            threshold_mv, self._range_id, self._scope.get_max_adc()
        )

        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(
                self._scope.get_chandle(),
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
        buffer = (ctypes.c_int16 * samples)()

        assert_pico_ok(
            ps.ps2000aSetDataBuffers(
                self._scope.get_chandle(),
                self._channel_id,
                buffer,
                None,
                self._total_samples,
                0,
                RATIO_MODE_NONE,
            )
        )

        self._buffer = buffer

    def bulk_buffer_create(self, samples: int, captures: int) -> None:
        """Add an acquisition buffer segment to a channel buffer."""
        buffer = []
        self._buffer = buffer

        for i in range(captures):
            segment = (ctypes.c_int16 * (samples))()

            self.buffer_add_segment(segment)

            assert_pico_ok(
                ps.ps2000aSetDataBuffer(
                    self._scope.get_chandle(),
                    self._channel_id,
                    segment,
                    self._total_samples,
                    i,
                    RATIO_MODE_NONE,
                )
            )

    def disable_trigger(self) -> None:
        """Disable a Picoscope channel trigger."""
        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(
                self._scope.get_chandle(), 0, 0, 0, 0, 0, 0
            )
        )

    def single_mv(self) -> np.ndarray:
        """Get a reading in millivolts from the channel buffer."""
        return np.ndarray(
            adc2mV(
                self._buffer,
                self._range_id,
                self._scope.get_max_adc(),
            )
        )

    def bulk_mv(self) -> np.ndarray:
        """Get an array of readings in millivolts from the channel buffer."""
        return np.array(
            [
                adc2mV(
                    adc_sample,
                    self._range_id,
                    self._scope.get_max_adc(),
                )
                for adc_sample in self._buffer
            ]
        )
