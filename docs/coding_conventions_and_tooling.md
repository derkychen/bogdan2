# Coding Conventions and Tooling

This document provides an overview of the conventions and tooling that are used in this project. It is recommended that these conventions be followed.

## Firmware

The firmware aims to follow Barr Group's *Embedded C Coding Standard*, which can be found [here](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf).

Doxygen-format comments were used throughout the firmware, though, as of now, Doxygen has not been used to generate documentation.

Enforcement of these conventions was done through the tool `clangd`, which handles code diagnostics and formatting.

## Host

The host-side functionality does not adhere to any particular standard. However, it aims to follow best practices for Python.

Google-style Python docstrings are used.

Enforcement of these conventions was done through:

* `ruff`: For formatting and surface-level linting.
* `basedpyright`: For comprehensive static type-checking.

## Miscellaneous

### Shell Scripts

The shell scripts in this project do not adhere to any particular standard. Enforcement of basic rules and formatting was done through `bashls` and `shfmt`.

### Markdown

The markdown files used for documentation in this project do not adhere to any particular standard. Formatting was done through `remark_ls`.

### TOML

The TOML files used for configuration in this project do not adhere to any particular standard. Basic rule-checking and formatting was done through `tombi`.
