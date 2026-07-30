# Bogdan 2: Revenge of the Industruino

## Dependencies:

You must have the following installed in order to set up Bogdan 2.

* `arm-none-eabi-gcc`
* `bossac`
* `cmake`
* `git`
* `ninja`
* `uv`
* [PicoSDK](https://www.picotech.com/library/our-oscilloscope-software-development-kit-sdk)
* [Thorlabs Kinesis](https://www.thorlabs.com/software-pages/motion_control)

If you are on Windows :disappointed:, you are probably sad, but also need to install [Git BASH](https://git-scm.com/install/windows).

## Setup

In the directory that you want the project to clone into, open `bash` and run

```bash
cd i/want/bogdan2/here # Replace with actual location.
git clone https://github.com/derkychen/bogdan2.git # Clone the repository.
cd bogdan2 # Change to the project directory.
chmod +x scripts/deps.sh # Permissions for a dependencies installation script.
scripts/deps.sh
```

## Usage

### Flash

To flash the firmware to the Industruino, double press the RST button on the back of the LCD screen and then open `bash` and run

```bash
scripts/firmware.sh -x -b release -p path/to/usb/port
```

Make sure to replace the port with the actual path to your USB port. On Linux this should be `/dev/tty*`. On macOS it should be `/dev/cu.usbmodem*`. On Windows it should be `COM*`. (`*` means "a sequence of any characters for any length".)

### API

#### Installation

To install the Bogdan 2 host API run (ideally in a virtual environment)

```bash
uv pip install -e i/put/bogdan2/here/bogdan2/host # Replace with actual location
```

The `-e` flag makes the Bogdan 2 installation is editable. This means if you edit the code in this repository, you do not need to rebuild or reinstall the package.

#### Usage

Refer to `examples/` for scripts you can copy-paste to run.

## Docs

Refer to `docs/` for more comprehensive documentation of the code.
