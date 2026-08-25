"""Example script for profiling a beam in Point-Count mode."""

from bogdan2.api.profiler import (
    AxisParams,
    BeamProfiler,
    BeamProfilerID,
    BeamProfilerParams,
    GridParams,
    PointCountCaptureParams,
)

x = AxisParams(
    min=-12,
    max=12,
    unit_mm=0.250,
    origin_mm=0.0,
)

y = AxisParams(
    min=-12,
    max=12,
    unit_mm=0.250,
    origin_mm=0.0,
)

grid = GridParams(x=x, y=y)

capture = PointCountCaptureParams(
    num_pulses=1,
    pretrigger_time_ns=200,
    posttrigger_time_ns=200,
    sample_interval_ns=4,
)

params = BeamProfilerParams(grid=grid, capture=capture)

id = BeamProfilerID(
    port="COM10",
    picoscope_serial_num="10338/0127",
    x_pdxc2_serial_num="112547939",
    y_pdxc2_serial_num="112512664",
)

with BeamProfiler(id) as p:
    profile = p.profile(params)

profile.plot()
print(profile.geometry())
