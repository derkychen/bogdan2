"""Example script for profiling a beam in `point_time` mode."""

from bogdan2.api.params import (
    AxisParams,
    GridParams,
    PointCountCaptureParams,
    ProfilerParams,
)
from bogdan2.api.profiler import Profiler

PORT = "COM5"
PICOSCOPE_SERIAL_NUM = "10338/0127"
X_PDXC2_SERIAL_NUM = "112547939"
Y_PDXC2_SERIAL_NUM = "112512664"

x = AxisParams(
    min=-5,
    max=5,
    unit_nm=250000,
    origin_nm=0,
)

y = AxisParams(
    min=-5,
    max=5,
    unit_nm=250000,
    origin_nm=0,
)

grid = GridParams(x=x, y=y)

capture = PointCountCaptureParams(
    num_pulses=3,
    pretrigger_time_ns=100,
    posttrigger_time_ns=100,
    sample_interval_ns=4,
)

params = ProfilerParams(grid=grid, capture=capture)

with Profiler(
    PORT, PICOSCOPE_SERIAL_NUM, X_PDXC2_SERIAL_NUM, Y_PDXC2_SERIAL_NUM
) as p:
    profile = p.profile(params)

if profile is not None:
    profile.plot2d()
    profile.plot3d()

    print(profile.geometry())
