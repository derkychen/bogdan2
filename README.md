# Bogdan 2: Revenge of the Industruino

This repository contains all the code used to set up and operate Bogdan 2, a beam profiler whose concept was conceived by Dr. Sascha Epp, our supervisor. Its purpose was mainly to solve two problems:

1. Difficulty triggering conventional beam profilers for very low frequency pulsed lasers.
2. Damage on detectors requiring the replacement of the entire beam profiler, which is costly and inefficient.

This beam profiler translates a photodiode in an $x\text{-}y$ plane, capturing intensities at different positions through an oscilloscope. It solves Problem 1 by using its own triggering mechanism that detects laser pulses. It constructs the beam profile across many pulses. It solves Problem 2 as the replacement of the photodiode as a detector is much less costly and easier than the replacement of an entire beam profiler.

## Dependencies:

You must be on Windows 10 or 11.

Dependencies for just usage:

* [Git BASH](https://git-scm.com/install/windows).
* [PicoSDK](https://www.picotech.com/library/our-oscilloscope-software-development-kit-sdk)
* [Thorlabs Kinesis](https://www.thorlabs.com/software-pages/motion_control)
* `python`
* `uv`

Dependencies for development:

* All of the above.
* `arm-none-eabi-gcc` (required)
* `bossac` (required)
* `clang-format` (optional)
* `clangd` (optional)
* `cmake` (required)
* `git` (required)
* `ninja` (required)

## Setup

In the directory that you want the project to clone into, open `bash` and run

```bash
cd i/want/bogdan2/here # Replace with actual location.
git clone https://github.com/derkychen/bogdan2.git # Clone the repository.
cd bogdan2 # Change to the project directory.
scripts/setup.sh # Run the setup script.
```

## Usage

### Flash

To flash the firmware to the Industruino, double press the RST button on the back of the LCD screen and then open `bash` and run

```bash
scripts/firmware.sh -p release -b -c path/to/usb/port
```

Make sure to replace the port with the actual path to your USB port. On Windows it should be `COMx` where `x` is a number.

### API

#### Installation

To install the Bogdan 2 host API run (ideally in a virtual environment)

```bash
uv pip install -e i/put/bogdan2/here/bogdan2/host # Replace with actual location
```

The `-e` flag makes the Bogdan 2 installation is editable. This means if you edit the code in this repository, you do not need to rebuild or reinstall the package.

#### Usage

Refer to `host/examples/` for scripts you can copy-paste to run.

## Documentation

Refer to `docs/` for more comprehensive documentation of the code.
