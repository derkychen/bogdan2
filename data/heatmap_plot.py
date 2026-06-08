
"""
Generates a 3d heatmap using a summary csv with the x,y positions and corresponding intensity measurements.
Height and colour correspond to intensity

"""

import csv
from pathlib import Path
import numpy as np

X_COLUMN = "x_target_nm"
Y_COLUMN = "y_target_nm"
INTENSITY_COLUMN = "peak_intensity_mV_avg"

def get_summary_csv():
    """
    Returns the path of the csv user wants to plot
    User chooses to get heatmap of latest experiment or specific experiment
    """

    experiment_root = Path("experiment_data")

    choice = input(
        "\nUse latest experiment? (y/n):"
    ).strip().lower()

    experiment_folders = [
            folder
            for folder in experiment_root.iterdir()
            if folder.is_dir()
        ]

    if not experiment_folders:
            raise FileNotFoundError("No experiment folders found.")
    
    if choice == "y":
        selected_folder = max(
            experiment_folders,
            key=lambda folder: folder.stat().st_mtime
        )

        summary_csv = (selected_folder / "beam_profile_summary.csv")


        return summary_csv

    else:
        print("\nAvailable experiments:")
        experiment_folders = sorted(
            [
                folder
                for folder in experiment_root.iterdir()
                if folder.is_dir()
            ]
        )

        for index, folder in enumerate(experiment_folders):
            print(f"{index+1}. {folder.name}")
        
        selection = int(input("\nSelect experiment number: "))
        selected_folder = experiment_folders[selection - 1]

    summary_csv = (selected_folder / "beam_profile_summary.csv")

    if not summary_csv.exists():
        raise FileNotFoundError(f"Could not find: {summary_csv}")
    
    print(f"\nUsing experiment:\n{selected_folder.name}")

    return summary_csv

def load_beam_profile_csv(csv_path, x_column=X_COLUMN, y_column=Y_COLUMN, intensity_column=INTENSITY_COLUMN):
    """
    Loads beam profile points from CSV file created by pico_save_csv_grid.py
    """

    #find the summary CSV file
    csv_path = Path(csv_path)
    if not csv_path.exists():                                                                       #check if file exists
        raise FileNotFoundError(f"Could not find CSV file: {csv_path}")
    
    x_values = []
    y_values = []
    intensity_values = []

    #reads CSV file
    with open(csv_path, "r", newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)                                                               #read each row in csv as a dictionary

        #check if all information is given in the CSV
        required_col = [x_column, y_column, intensity_column]
        missing_columns = [col for col in required_col if col not in reader.fieldnames]

        if missing_columns:                                                                         #error if any missing columns
            raise KeyError(
                f"Missing column(s): {missing_columns}\n"
                f"Available columns are: {reader.fieldnames}"
            )
        
        #add values to list to be turned into a grid for matplotlit
        for row in reader:                                                                          #loop through each row in CSV
            x_values.append(float(row[x_column]))
            y_values.append(float(row[y_column]))
            intensity_values.append(float(row[intensity_column]))
    
    return (
        np.array(x_values, dtype=float),
        np.array(y_values, dtype=float),
        np.array(intensity_values, dtype=float)
    )

def make_regular_grid(x, y, intensity):
    """
    Reshapes scattered x,y,intensity points into a rectangular grid that can be read by matplotlib
    Checks if any points are missing
    """

    #finds x and y dimensions of grid
    unique_x = np.unique(x)
    unique_y = np.unique(y)

    expected_num_points = len(unique_x) * len(unique_y)

    if len(x) != expected_num_points:
        return None
    
    #create empty grid for intensity values
    #rows are y values, columns are x values
    Z_grid = np.full((len(unique_y), len(unique_x)), np.nan)

    #fill grid
    for x_value, y_value, z_value in zip(x, y, intensity):
        x_index = np.where(unique_x == x_value)[0][0]
        y_index = np.where(unique_y == y_value)[0][0]
        Z_grid[y_index, x_index] = z_value
    
    #if any location in grid is empty, grid is incomplete
    if np.isnan(Z_grid).any():
        return None
    
    X_grid, Y_grid = np.meshgrid(unique_x, unique_y)
    return X_grid, Y_grid, Z_grid

def plot_3d_beam_heatmap(X_grid, Y_grid, Z_grid, title="3D Beam Profile Heatmap", save_path = None):
    """
    Creates 3d heatmap 

    surface height shows intensity
    surface colour shows intensity
    """

    import matplotlib.pyplot as plt

    fig = plt.figure(figsize=(10,8))
    ax = fig.add_subplot(111, projection="3d")
    
    surface = ax.plot_surface(
        X_grid,
        Y_grid,
        Z_grid,
        cmap="inferno"
    )

    ax.set_xlabel("X Position")
    ax.set_ylabel("Y Position")
    ax.set_zlabel("Intensity (mV)")

    ax.set_title(title)

    #legend
    fig.colorbar(                                           
        surface,
        ax=ax,
        label="Intensity (mV)"
    )

    if save_path is not None:
        plt.savefig(save_path, dpi=300, bbox_inches="tight")
        print(f"Saved 3D heatmap to: {save_path}")
    
    plt.show()

def main():

    csv_path = get_summary_csv()
    print(f"\nLoading summary file:\n{csv_path}\n")

    x, y, intensity = load_beam_profile_csv(csv_path)

    grid_result = make_regular_grid(x, y, intensity)

    if grid_result is None:
        print("Could not create a complete grid.")
        return
    
    X_grid, Y_grid, Z_grid = grid_result
    
    plot_3d_beam_heatmap(
        X_grid,
        Y_grid,
        Z_grid
    )

    '''
    print("x:", x)
    print("y:", y)
    print("intensity:", intensity)
    print("number of points:", len(x))
    '''

if __name__ == "__main__":
    main()