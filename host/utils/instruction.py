"""Instruction parsing module."""

from typing import Any, Final

MOVEMENT_MODE_POINT: Final[int] = 0
MOVEMENT_MODE_TIME: Final[int] = 0
MOVEMENT_MODE_CONTINUOUS: Final[int] = 0


class InvalidModeException(Exception):
    """Exception for when the provided mode is not valid."""


class InstructionFieldNoneException(Exception):
    """Exception for when a field of an instruction is `None`."""


def _ceil_div(num: int, denom: int) -> int:
    """Ceiling division between two integers."""
    return -(num // -denom)


def check_no_none(instruction: dict[str, Any]) -> None:
    """Traverse all instruction fields and check that they are not `None`."""
    for key, val in instruction.items():
        if val is None:
            raise InstructionFieldNoneException(
                f"Instruction field {key} is `None`."
            )

        if isinstance(val, dict):
            check_no_none(val)


def parse_indio_instruction(instruction: dict[str, Any]) -> dict[str, int]:
    """Parse instructions into a flat JSON for the Industruino IND.I/O."""
    indio_instruction = {}

    mode = instruction["mode"]

    if type(mode is str):
        match mode.lower():
            case "point":
                indio_instruction["mode"] = MOVEMENT_MODE_POINT
            case "time":
                indio_instruction["mode"] = MOVEMENT_MODE_TIME
            case "continuous":
                indio_instruction["mode"] = MOVEMENT_MODE_CONTINUOUS
            case _:
                raise InvalidModeException("Mode provided is not valid.")

    elif type(mode is int) and mode in (
        MOVEMENT_MODE_POINT,
        MOVEMENT_MODE_TIME,
        MOVEMENT_MODE_CONTINUOUS,
    ):
        indio_instruction["mode"] = mode

    else:
        raise Exception("Mode provided is not valid.")

    indio_instruction["x_min"] = instruction["grid"]["x"]["min"]
    indio_instruction["x_max"] = instruction["grid"]["x"]["max"]
    indio_instruction["x_unit_nm"] = instruction["grid"]["x"]["unit_nm"]
    indio_instruction["x_origin_nm"] = instruction["grid"]["x"]["origin_nm"]
    indio_instruction["y_min"] = instruction["grid"]["y"]["min"]
    indio_instruction["y_max"] = instruction["grid"]["y"]["max"]
    indio_instruction["y_unit_nm"] = instruction["grid"]["y"]["unit_nm"]
    indio_instruction["y_origin_nm"] = instruction["grid"]["y"]["origin_nm"]
    indio_instruction["num_pulses"] = instruction["capture"]["num_pulses"]
    indio_instruction["posttrigger_time_us"] = _ceil_div(
        instruction["capture"]["posttrigger_time_ns"], 1000
    )

    check_no_none(indio_instruction)

    return indio_instruction
