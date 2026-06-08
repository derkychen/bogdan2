""" Imports PicoBlockMode, captures all configured picoscope channels

    Will output 3 main types of CSVs:
    One waveform CSV per pulse that gives all the voltage measurements (samples) of each configured channel (gives all the raw data)
    pulse_summary CSV gives averaged position and peak intensity for each pulse
    beam_profile_summary CSV averages position and peak intensity over the number of pulses per point

"""

import csv
import numpy as np
from datetime import datetime
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parents[1]))

from pico.pico_config import PICO_CONFIG
from pico.block_mode import PicoBlockMode
from pico.setup import load_settings

BASE_OUTPUT_FOLDER = "experiment_data"

INTENSITY_CHANNEL = "intensity"
X_POSITION_CHANNEL = "x_position"
Y_POSITION_CHANNEL = "y_position"

def create_experiment_folder():
    """
    Creates unique folder for ecah experiment run.
    """

    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

    experiment_folder = (
        Path(BASE_OUTPUT_FOLDER)
        / f"experiment_{timestamp}"
    )

    experiment_folder.mkdir(parents=True, exist_ok=True)

    return experiment_folder

def load_instr():
    """
    Loads JSON file
    """

    instruction = load_settings(PICO_CONFIG["settings_json_path"])
    required_grid_keys = [
        "x_min", "x_max", "x_unit_nm",
        "y_min", "y_max", "y_unit_nm",
        "num_pulses"
    ]

    missing = [key for key in required_grid_keys if key not in instruction]
    if missing:
        raise KeyError(f"Missing required grid JSON setting(s): {missing}")

    return instruction

def generate_grid_points(instruction):
    """
    Converts unit coordinates in JSON into target nanometer coordinates
    """
    
    x_values = list(range(instruction["x_min"], instruction["x_max"]+1))
    y_values = list(range[instruction["y_min"], instruction["y_max"]+1])

    point_index = 0

    for y_unit in y_values:
        for x_unit in x_values:
            point_index +=1

            yield {
                "point index": point_index,
                "x_unit": x_unit,
                "y_unit": y_unit,
                "x_target_nm": x_unit * instruction["x_unit_nm"],
                "y_target_nm": y_unit * instruction["y_unit_nm"]
            }

def save_pulse_csv(pico, data_mV, filename):
    with open(filename, "w", newline="") as file:
        writer = csv.writer(file)

        header = ["sample"]
                
        for channel_name in pico.channels.keys():
            header.append(f"{channel_name}_mV")
        writer.writerow(header)

        for sample_index in range(pico.total_samples):
            row = [sample_index]

            for channel_name in pico.channels.keys():
                row.append(data_mV[channel_name][sample_index])

            writer.writerow(row)

    print(f"Saved {filename}")

def make_summary_row(pico, data_mV, grid_point, pulse_index):
    start = pico.pre_trigger_samples

    peak_intensity_mV = float(
        np.max(data_mV[INTENSITY_CHANNEL][start:])
    )
    x_position_mV = float(
        np.mean(data_mV[X_POSITION_CHANNEL][start:])
    )
    y_position_mV = float(
        np.mean(data_mV[Y_POSITION_CHANNEL][start:])
    )


    #convert from mV to nm (6mm per 10V)
    x_position_nm = x_position_mV * 600
    y_position_nm = y_position_mV * 600

    return {
        "point_index": grid_point["point_index"],
        "pulse": pulse_index + 1,
        "x_position_nm": x_position_nm,
        "y_position_nm": y_position_nm,
        "peak_intensity_mV": peak_intensity_mV
    }

def average_point_rows(rows_for_one_point):
    """
    Makes an averaged beam-profile row from all pulses at the same grid point
    """

    first = rows_for_one_point[0]

    return{
        "point_index": first["point_index"],
        "x_nm_avg": float(np.mean([row["x_nm"] for row in rows_for_one_point])),
        "y_nm_avg": float(np.mean([row["y_nm"] for row in rows_for_one_point])),
        "num_pulses": len(rows_for_one_point),
        "peak_intensity_mV": float(np.max([r["peak_intensity_mV"] for r in rows_for_one_point])),
    }

def write_dict_csv(filename, rows, fieldnames):
    with open(filename, "w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    
    print(f"Save {filename}")

def main():
    instruction = load_instr()
    grid_points = list(generate_grid_points(instruction))

    pulses_per_point = instruction["num_pulses"]
    total_points = len(grid_points)
    total_captures = total_points * pulses_per_point

    output_path = create_experiment_folder()
    
    print(f"\nSaving experiment to: {output_path}\n")
    print(f"Grid points: {total_points}")
    print(f"Pulses per point: {pulses_per_point}")
    print(f"Total captures needed: {total_captures}\n")

    pico = PicoBlockMode(PICO_CONFIG)

    all_pulse_rows = []                                                 #stores one row per pulse
    averaged_point_rows = []                                            #stores one averaged row per grid point

    try:
        pico.setup()

        pico.num_pulses = total_captures
        print("Starting one full-grid rapid-block capture.")

        buffers = pico.create_rapid_block_buffers()
        pico.run_rapid_block_capture()

        #go through every captured pulse
        for global_capture_index in range(total_captures):
            #find which grid point this pulse belongs to
            point_list_index = global_capture_index // pulses_per_point
            grid_point = grid_points[point_list_index]

            #find which pulse number it is at this point
            pulse_index_inside_point = global_capture_index % pulses_per_point
            point_index = grid_point["point_index"]

            #folder for each point (includes all pulses for each point)
            point_folder = output_path / (f"point_{point_index:04d}_x_{grid_point['x_unit']}_y_{grid_point['y_unit']}")
            point_folder.mkir(parents=True, exist_ok=True)

            data_mV = pico.convert_capture_to_mV(buffers, capture_index=global_capture_index)

            #save full waveform CSV for pulse
            pulse_filename = point_folder / f"pulse_{pulse_index_inside_point + 1:03d}.csv"
            save_pulse_csv(pico, data_mV, pulse_filename)

            summary_row = make_summary_row(pico, data_mV, grid_point, pulse_index_inside_point)

            summary_row["global_capture"] = global_capture_index + 1

            all_pulse_rows.append(summary_row)
        
        #all pulses are processed, now group every num_pulses rows into one averaged point
        for point_list_index in range(total_points):
            start_index = point_list_index * pulses_per_point
            end_index = start_index + pulses_per_point

            rows_for_this_point = all_pulse_rows[start_index:end_index]

            averaged_point_rows.append(
                average_point_rows(rows_for_this_point)
            )

        #save every pulse result
        pulse_summary_filename = output_path / "pulse_summary.csv"
        write_dict_csv(
            pulse_summary_filename,
            all_pulse_rows,
            fieldnames=[
                "global_capture",
                "point_index",
                "pulse",
                "x_nm",
                "y_nm",
                "peak_intensity_mV"
            ]
        )

        #save one averaged result per grid point
        beam_summary_filename = output_path / "beam_profile_summary.csv"
        write_dict_csv(
            beam_summary_filename,
            averaged_point_rows,
            fieldnames = [
                "point_index",
                "num_pulses",
                "x_nm_avg",
                "y_nm_avg",
                "peak_intensity_mV_max"
            ]
        )

        print("Full-grid capture complete")

    finally:
        pico.close()


if __name__ == "__main__":
    main()



