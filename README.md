# Bogdan 2: Rise of the Pico

## Dependencies:

You must have the following installed in order to set up Bogdan 2.

* `uv`
* `git`
* `python3`
* PicoScope [PicoSDK](https://www.picotech.com/library/our-oscilloscope-software-development-kit-sdk)

If you are on Windows :disappointed:, you are probably sad, but also need to install [Git Bash](https://git-scm.com/install/windows).

## Setup

In the directory that you want the project to reside in, open `bash` or another Unix shell and run

```bash
git clone https://github.com/derkychen/bogdan2.git # Clone the repository
cd bogdan2 # Change to the project directory
chmod +x deps.sh # Permissions for a dependencies installation script
./deps.sh
```

## Usage

### Calibration

In the project directory, run

```bash
./bogdan.sh calibrate
```

to run Bogdan 2's calibration script.

### Profiling

In the project directory, run

```bash
./bogdan.sh profile path/to/your/instruction.json
```

to profile the beam.
