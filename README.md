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

If you are on Windows :disappointed:, you are probably sad, but also need to install the following:

* [Git Bash](https://git-scm.com/install/windows).

## Setup

In the directory that you want the project to reside in, open `bash` or another Unix shell and run

```bash
git clone https://github.com/derkychen/bogdan2.git # Clone the repository
cd bogdan2 # Change to the project directory
chmod +x scripts/deps.sh # Permissions for a dependencies installation script
scripts/deps.sh
```
