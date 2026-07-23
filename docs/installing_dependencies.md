# Installing Dependencies

## Windows

### Git and Git BASH

To install Git and Git BASH follow this link:
<https://git-scm.com/install/windows>.

NOTE: This may already be installed on your system.

An easy way to run Git BASH is to go to VSCode set your terminal to use Git BASH:

1. `Ctrl+Shift+P` to open the Command Palette
2. From the Command Palette, navigate to "Terminal: Select Default Profile"

Then select Git BASH.

### Command-Line Dependencies With WinGet

On Windows 10 or 11 `winget` is installed by default. In Command Prompt, Windows Powershell, or Git BASH (preferably, since all other setup is run there) run:

```bash
winget install -e --id Kitware.CMake # `cmake`
winget install -e --id Ninja-build.Ninja # `ninja`
winget install --id=astral-sh.uv  -e # `uv`
```

### Other Dependencies

#### Command-Line Tools

To install `arm-none-eabi-gcc`, follow this link:
<https://gitlab.arm.com/tooling/gnu-toolchains-for-arm/-/tree/releases/15.3.rel1>

Download `arm-gnu-toolchain-15.3.rel1-mingw-w64-x86_64-arm-none-eabi.msi`. Run the installer, and install in the default location.

To install `bossac`, follow this link:
<https://github.com/shumatech/BOSSA/releases/tag/1.9.1>

Download `bossa-x64-1.9.1.msi`. Run the installer, and install in the default location.

To actually use these commands, you must locate the binaries and append them to the `PATH` environment variable. If you installed in the default location, run these in Git BASH:

```bash
echo 'export PATH="$PATH:/c/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin"' >> ~/.bash_profile
echo 'export PATH="$PATH:/c/Program Files (x86)/BOSSA"' >> ~/.bash_profile
```

Phis creates a file called `~/.bash_profile`. The scripts in this file are automatically run by Git BASH. In this case, the scripts append location of the binaries to your `PATH`.

If future setup does not work, it is very likely that location of the binaries is incorrect, edit the file to instead reference correct locations.

#### Applications

To install PicoSDK, follow this link:
<https://www.picotech.com/library/our-oscilloscope-software-development-kit-sdk>

To install Thorlabs Kinesis, follow this link:
<https://www.thorlabs.com/software-pages/motion_control>

## Next Steps

Follow the rest of the README.
