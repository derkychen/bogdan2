"""Example script for profiling a beam in `point_time` mode."""

from api.params import (
    AxisParams,
    GridParams,
    PointTimeCaptureParams,
    ProfilerParams,
)
from api.profiler import Profiler

PORT = "COM5"

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

capture = PointTimeCaptureParams(
    wait_time_us=50000,
    pretrigger_time_ns=100,
    posttrigger_time_ns=100,
    sample_interval_ns=4,
)

params = ProfilerParams(grid=grid, capture=capture)

with Profiler() as p:
    p.profile(PORT, params)
