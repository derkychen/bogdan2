"""
Trigger is disabled.
Picoscope will capture at regular intervals 
(used to time it with the laser pulse)
"""
import argparse
import time
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parents[2]))


from picosdk.functions import adc2mV, assert_pico_ok
from picosdk.ps2000a import ps2000a as ps


from pico.pico_config import PICO_CONFIG

from pico.setup import load_settings

from pico.no_trigger_acquisition import(
    setup_no_trigger_block_capture,
    capture_single_no_trigger_block,
    close_acquisition
)

from data.save.csv_helpers import(
    load_instr,
    create_experiment_folder,
    generate_grid_points,
    save_pulse_csv,
    make_summary_row,
    average_point_rows,
    write_dict_csv
)

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--interval_ms",
        type=float,
        required=True,
        help="Time between capture starts in milliseconds"
    )

    args = parser.parse_args()

    instruction = load_instr()
    grid_points = list(generate_grid_points(instruction))

    pulses_per_point = instruction["num_pulses"]
    total_points = len(grid_points)
    total_captures = total_points * pulses_per_point

    interval_s = args.interval_ms / 1000.0

    output_path = create_experiment_folder(prefix = "timed_no_trigger_grid")

    print(f"\nSaving experiment to: {output_path}\n")
    print(f"Grid points: {total_points}")
    print(f"Pulses per point: {pulses_per_point}")
    print(f"Total captures needed: {total_captures}\n")

    acq = None

    all_pulse_rows = []
    averaged_point_rows = []

    try:
        acq = setup_no_trigger_block_capture(PICO_CONFIG)

        start_time = time.perf_counter()
        global_capture_index = 0

        for grid_point in grid_points:
            point_index = grid_point["point_index"]

            point_folder = output_path / (
                f"point_{point_index:04d}"
                f"_x_{grid_point['x_unit']}"
                f"_y_{grid_point['y_unit']}"
            )

            point_folder.mkdir(parents=True, exist_ok=True)
            rows_for_this_point = []

            for pulse_index in range(pulses_per_point):
                target_start_time = start_time + global_capture_index * interval_s

                now = time.perf_counter()

                if now < target_start_time:
                    time.sleep(target_start_time - now)
                
                actual_start_time = time.perf_counter()

                print(
                    f"\nPoint {point_index}/{total_points},"
                    f"Pulse {pulse_index + 1}/{pulses_per_point}"
                )

                data_mV, samples_returned = capture_single_no_trigger_block(acq)

                pulse_filename = (point_folder / f"pulse_{pulse_index + 1:03d}.csv")

                save_pulse_csv(acq, data_mV, pulse_filename)

                summary_row = make_summary_row(
                    acq,
                    data_mV,
                    pulse_index,
                    grid_point = grid_point
                )

                all_pulse_rows.append(summary_row)
                rows_for_this_point.append(summary_row)

                global_capture_index += 1

            averaged_point_rows.append(
                average_point_rows(rows_for_this_point)
            )
        
        pulse_summary_filename = output_path / "pulse_summary.csv"

        write_dict_csv(
            pulse_summary_filename,
            all_pulse_rows,
            fieldnames=[
                "point_index",
                "pulse",
                "x_nm",
                "y_nm",
                "peak_intensity_mV",
            ]
        )

        beam_summary_filename = output_path / "beam_profile_summary.csv"

        write_dict_csv(
            beam_summary_filename,
            averaged_point_rows,
            fieldnames = [
                "point_index",
                "num_pulses",
                "x_nm_avg",
                "y_nm_avg",
                "peak_intensity_mV_avg"
            ]
        )

        print("\nNo-trigger grid capture complete.")

    finally:
        close_acquisition(acq)

if __name__ == "__main__":
    main()