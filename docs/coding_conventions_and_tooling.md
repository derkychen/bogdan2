# Coding Conventions and Tooling

This document provides an overview of the conventions and tooling that are used in this project. It is recommended that these conventions be followed.

## C

The firmware aims to follow Barr Group's *Embedded C Coding Standard*, which can be found [here](https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf).

Doxygen-format comments were used throughout the firmware, though, as of now, Doxygen has not been used to generate documentation.

Enforcement of these conventions was done through the tool `clangd`, which handles code diagnostics and formatting.

Functions are namespaced according to their functionality, which is the same as their module name.

## Python

Numpy-style Python docstrings are used.

Enforcement of these conventions was done through:

* `ruff` for formatting and surface-level linting.
* `basedpyright` for comprehensive static type-checking.

Other conventions:

* Literals with more than 5 digits are separated with underscores for readability.

* Some mandatory arguments are also keyword arguments for better readability.

## Miscellaneous

### Shell Scripts

Enforcement of basic rules and formatting was done through `bashls` and `shfmt`.

### Markdown

Formatting was done through `remark_ls`.

### TOML

Basic rule-checking and formatting was done through `tombi`.
