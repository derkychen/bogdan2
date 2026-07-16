# Issues We Encountered

This document contains issues we encountered while setting up, using, or maintaining this repository. Feel free to add to this document.

## Common Issues

### Command not found

#### Problem

When you install one of the dependencies and you try to run the command in BASH and it says "command not found"

#### Fix

If you are on Windows, search for "Edit the system environment variables". You might need administrator permissions. Navigate to "Environment Variables > Path" and add a new entry, which is the full path to the **folder** in which the command you want to run is in.

If you are on Linux/macOS, add `export PATH="$PATH:path/to/the/command` to your `~/.bashrc`. Create the file if it does not exist.

### Python modules not found error

#### Problem

When you are running a Python script and you encounter `ModuleNotFoundError: No module named 'module_i_want_to_run`.

#### Fix

Make sure that

1. You are inside the project directory. If running `pwd` in BASH doesn't show something like `c/Users/me/bogdan2` you need to change directory to the project.

2. You have activated the Python virtual environment. Run

```bash
source .venv/bin/activate # If you are on Linux/macOS
source .venv/Scripts/activate # If you are on Windows
```

## Weird Issues

### `arm-none-eabi-gcc` command not found

#### Problem

When installing `arm-none-eabi-gcc`, we found difficulty configuring the windows `PATH` environment variable such that Git BASH would recognize it as a command.

#### Fix

Add `export PATH="$PATH:path/to/arm-none-eabi-gcc"` to `~/.bashrc` (make sure to replace the placeholder path).

### Symlink issue when running `scripts/deps.sh`

#### Problem

When running `scripts/deps.sh` from the Windows H drive we encountered errors that indicated too many levels of symbolic links.

#### Fix

Run `scripts/deps.sh` from the Windows C drive (which is where the repository is actually located on the file system) to avoid symbolic link weirdness.
