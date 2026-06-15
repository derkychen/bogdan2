import csv
import numpy as np
from datetime import datetime
from pathlib import Path

from pico.setup import load_settings
from pico.pico_config import PICO_CONFIG

BASE_OUTPUT_FOLDER = "experiment_data"
INTENSITY_CHANNEL = "intensity"
X_POSITION_CHANNEL = "x_position"
Y_POSITION_CHANNEL = "y_position"

def get_attr(obj, key):
    if isinstance(obj, dict):
        return obj[key]
    return getattr(obj, key)

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
    y_values = list(range(instruction["y_min"], instruction["y_max"]+1))

    point_index = 0

    for y_unit in y_values:
        for x_unit in x_values:
            point_index +=1

            yield {
                "point_index": point_index,
                "x_unit": x_unit,
                "y_unit": y_unit,
                "x_target_nm": x_unit * instruction["x_unit_nm"],
                "y_target_nm": y_unit * instruction["y_unit_nm"]
            }

def create_experiment_folder(prefix = "experiment"):
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

def save_pulse_csv(pico_or_acq, data_mV, filename):
    channels = get_attr(pico_or_acq, "channels")
    total_samples = get_attr(pico_or_acq, "total_samples")

    with open(filename, "w", newline="") as file:
        writer = csv.writer(file)

        header = ["sample"]
                
        for channel_name in channels.keys():
            header.append(f"{channel_name}_mV")
        writer.writerow(header)

        for sample_index in range(total_samples):
            row = [sample_index]

            for channel_name in channels.keys():
                row.append(data_mV[channel_name][sample_index])

            writer.writerow(row)

    print(f"Saved {filename}")

def write_dict_csv(filename, rows, fieldnames):

    with open(filename, "w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    
    print(f"Save {filename}")

def make_summary_row(pico, data_mV, pulse_index, intensity_channel="intensity", x_position_channel = "x_position", y_position_channel = "y_position", grid_point=None):
    
    if isinstance(pico,dict):
        start = pico["pre_trigger_samples"]
    else:
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


    #convert from mV to nm (6mm per 10V = 600nm per 1mV)
    x_position_nm = x_position_mV * 600
    y_position_nm = y_position_mV * 600

    row = {
        "pulse": pulse_index + 1,
        "x_nm": x_position_nm,
        "y_nm": y_position_nm,
        "peak_intensity_mV": peak_intensity_mV
    }

    if grid_point is not None:
        row.update(
            {
                "point_index": grid_point["point_index"],
            }
        )
    
    return row

def average_point_rows(rows_for_one_point):
    """
    Makes an averaged beam-profile row from all pulses at the same grid point
    """

    first = rows_for_one_point[0]

    return{
        "point_index": first["point_index"],
        "num_pulses": len(rows_for_one_point),
        "x_nm_avg": float(np.mean([row["x_nm"] for row in rows_for_one_point])),
        "y_nm_avg": float(np.mean([row["y_nm"] for row in rows_for_one_point])),
        "num_pulses": len(rows_for_one_point),
        "peak_intensity_mV_avg": float(np.max([row["peak_intensity_mV"] for row in rows_for_one_point])),
    }