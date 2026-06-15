""" Captures all configured picoscope channels on a trigger
    (used for AND gate approach, and continuous scan)

    Will output 3 main types of CSVs:
    One waveform CSV per pulse that gives all the voltage measurements (samples) of each configured channel (gives all the raw data)
    pulse_summary CSV gives averaged position and peak intensity for each pulse
    beam_profile_summary CSV averages position and peak intensity over the number of pulses per point

"""

import sys
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parents[2]))

from pico.pico_config import PICO_CONFIG
from pico.block_mode import PicoBlockMode
from pico.setup import load_settings

from pico.save.csv_helpers import(
    load_instr,
    create_experiment_folder,
    generate_grid_points,
    save_pulse_csv,
    make_summary_row,
    average_point_rows,
    write_dict_csv
)

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

        for grid_point in grid_points:
            point_index = grid_point["point_index"]

            pico.num_pulses = pulses_per_point

            buffers = pico.create_rapid_block_buffers()
            pico.run_rapid_block_capture()

            point_folder = output_path / (
                f"point_{point_index:04d}"
                f"_x_{grid_point['x_unit']}"
                f"_y_{grid_point['y_unit']}"
            )
            point_folder.mkdir(parents=True, exist_ok=True)
            rows_for_this_point = []

            for pulse_index in range(pulses_per_point):
                data_mV = pico.convert_capture_to_mV(buffers, capture_index=pulse_index)

                pulse_filename = (point_folder / f"pulse_{pulse_index + 1:03d}.csv")
                save_pulse_csv(pico, data_mV, pulse_filename)

                summary_row = make_summary_row(pico, data_mV, pulse_index, grid_point=grid_point)

                all_pulse_rows.append(summary_row)
                rows_for_this_point.append(summary_row)

            averaged_point_rows.append(
                average_point_rows(rows_for_this_point)
            )
                
        '''
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
        
        '''
            

        #save every pulse result
        pulse_summary_filename = output_path / "pulse_summary.csv"
        write_dict_csv(
            pulse_summary_filename,
            all_pulse_rows,
            fieldnames=[
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
                "peak_intensity_mV_avg"
            ]
        )

        print("Full-grid capture complete")

    finally:
        pico.close()


if __name__ == "__main__":
    main()



