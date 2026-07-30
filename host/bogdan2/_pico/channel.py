"""Abstraction for a PicoScope channel."""

from __future__ import annotations

import ctypes
from typing import Final

import numpy as np
import numpy.typing as npt
from picosdk.functions import adc2mV, assert_pico_ok, mV2adc
from picosdk.ps2000a import ps2000a as ps

from bogdan2._pico.constants import RATIO_MODE_NONE

COUPLING_DC: Final[int] = ps.PS2000A_COUPLING["PS2000A_DC"]


class Channel:
    """Abstraction for a channel of the PicoScope."""

    def __init__(
        self,
        chandle: ctypes.c_int16,
        max_adc: ctypes.c_int16,
        name: str,
        channel_id: int,
        range_id: int,
    ) -> None:
        """Initialize a PicoScope channel."""
        self._chandle: ctypes.c_int16 = chandle
        self._max_adc: ctypes.c_int16 = max_adc

        self.name: str = name

        self._channel_id: int = channel_id
        self._range_id: int = range_id

        self._single_buffer: ctypes.Array[ctypes.c_int16]
        self._bulk_buffers: list[ctypes.Array[ctypes.c_int16]]

        assert_pico_ok(
            ps.ps2000aSetChannel(
                self._chandle,
                self._channel_id,
                1,
                COUPLING_DC,
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
        threshold_mv: float = 2000.0,
    ) -> None:
        """Configure a PicoScope channel as a logical trigger."""
        trigger_adc: ctypes.c_int16 = mV2adc(
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
        buffer = (ctypes.c_int16 * samples)()

        assert_pico_ok(
            ps.ps2000aSetDataBuffers(
                self._chandle,
                self._channel_id,
                buffer,
                None,
                samples,
                0,
                RATIO_MODE_NONE,
            )
        )

        self._single_buffer = buffer

    def bulk_buffer_create(self, samples: int, captures: int) -> None:
        """Add an acquisition buffer segment to a channel buffer."""
        buffers = []
        self._bulk_buffers = buffers

        for i in range(captures):
            segment = (ctypes.c_int16 * (samples))()

            self._bulk_buffers.append(segment)

            assert_pico_ok(
                ps.ps2000aSetDataBuffer(
                    self._chandle,
                    self._channel_id,
                    segment,
                    samples,
                    i,
                    RATIO_MODE_NONE,
                )
            )

    def disable_trigger(self) -> None:
        """Disable a Picoscope channel trigger."""
        assert_pico_ok(
            ps.ps2000aSetSimpleTrigger(self._chandle, 0, 0, 0, 0, 0, 0)
        )

    def single_mv(self) -> npt.NDArray[np.float64]:
        """Get a reading in millivolts from the channel buffer."""
        return np.array(
            adc2mV(
                self._single_buffer,
                self._range_id,
                self._max_adc,
            )
        )

    def bulk_mv(self) -> list[npt.NDArray[np.float64]]:
        """Get an array of readings in millivolts from the channel buffer."""
        return [
            np.array(
                adc2mV(
                    adc_sample,
                    self._range_id,
                    self._max_adc,
                )
            )
            for adc_sample in self._bulk_buffers
        ]
