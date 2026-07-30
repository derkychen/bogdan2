# Repository Structure

This document explains the structure of the repository. Since individual files have details contained in comments/docstrings at the top, this document only explains directory-scope details.

## `docs/`

If you are reading this, you are under `docs/` right now. This directory contains a collection of Markdown files that contain documentation on the project.

## `firmware/`

This directory contains code that runs on the Industruino IND.I/O microcontroller, mainly written in C.

Underneath each code directory, there are `include/` and `src/` directories, which contain header and source files respectively. There is some intentional nesting in some of the `include/` directories, which allows namespacing of includes.

### `app/`

This directory is the primary location that concerns maintainers of this codebase. It defines the application behaviour and logic. It contains sort-of object-oriented abstractions of hardware and concepts used by the beam profiler.

### `board/indio/`

This directory contains utilities that pertains to the IND.I/O baseboard. The only baseboard functionality that is used is analog output. This module also provides functions to initialize hardware, and provides global variables that are used by other modules to interact with board hardware.

### `cmake/`

This directory contains CMake-related functionality. CMake is used to build the C project into a binary that can be flashed to the IND.I/O. The directory contains primarily wrappers for external dependencies, as well as compiling configurations.

#### `external/`

This directory contains CMake wrappers for external dependencies, which are CMSIS and CMSIS Atmel headers (which define macros used in the code to access registers, peripherals, etc.), JSMN (a JSON parsing utility), and TinyUSB, which provides serial communication functionality.

#### `mcu/`

This directory contains compile options for the SAMD21G18A processor, which is the processor on board the IND.I/O.

#### `options/`

This directory contains warning and feature-related compile options. The CMakeLists Files in other locations use the features and warnings to ensure that errors/warnings are largely ignored for external dependencies (since this is code that is not written by us), and tightens checking on code in this repository so that any warning causes compilation to fail.

### `drivers/`

This directory contains drivers for chips on board the IND.I/O baseboard. These chips are used to control the I<sup>2</sup>C devices that are responsible for driving the analog outputs on the baseboard.

### `external/`

This directory houses the Git submodules that the firmware depends on.

### `platform/samd21g18a`

This directory contains low-level drivers for the SAMD21G18A processor. It is important to differentiate these drivers from the IND.I/O baseboard drivers, as they are independent.

### `usb/`

This directory contains TinyUSB-related utilities that are miscellaneous and do not really belong anywhere else. These contains TinyUSB configuration options and USB descriptors.

## `host/`

This directory contains host-side functionality, mainly written in Python. Whereas the microcontroller is a "slave" that waits for commands, it is the host (computer) that initiates operations such as calibration or profiling.

### `bogdan2/`

This directory contains the main Python package functionality. When `from bogdan2 import ...` is run, functionality from this directory is called.

#### `_pdxc2/`

This directory contains a private module that provides functionality for controlling the Thorlabs PDXC2 controllers, which act as an interface between the microcontroller/computer and the stage that is actually moving. This module is limited in its control of movement (that is the microcontroller's job), its main purpose is to configure the controllers so that they can be controlled by the microcontroller.

#### `_pico/`

This directory contains a private module that provides functionality for acquisition via the PicoScope oscilloscope, which is the device that acts in synchronization with the stages to capture beam measurements.

#### `_utils/`

This directory contains a private module that provides one function: the ceiling division of integers. This function is preferred over `math.ceil` as it retains precision by not introducing floating-point error. This module might be removed if deemed not necessary.

#### `api/`

This directory contains the main API that is used by those who want to write scripts to use/automate beam profiling tasks.

### `examples/`

This directory contains example scripts that perform measurements. They mainly serve as a tutorial for users of the API.

### `typings/`

This directory provides exclusively stub files for type-checking the Python code written in `bogdan2/`

## `scripts/`

This directory contains shell scripts that automate some project-related tasks (e.g. the installation of dependencies and flashing of firmware).
