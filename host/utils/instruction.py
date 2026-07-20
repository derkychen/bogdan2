"""Instruction parsing module."""

from typing import Any, Final

MODE_POINT: Final[int] = 0
MODE_TIME: Final[int] = 1
MODE_CONTINUOUS: Final[int] = 2

MODE_MAP = {
    "point": MODE_POINT,
    "time": MODE_TIME,
    "continuous": MODE_CONTINUOUS,
}


class InvalidMode(Exception):
    """Exception for when the provided mode is not valid."""


class InstructionFieldNone(Exception):
    """Exception for when a field of an instruction is `None`."""


def _ceil_div(num: int, denom: int) -> int:
    """Ceiling division between two integers."""
    return -(num // -denom)


def check_no_none(instruction: dict[str, Any]) -> None:
    """Traverse all instruction fields and check that they are not `None`."""
    for key, val in instruction.items():
        if val is None:
            raise InstructionFieldNone(f"Instruction field {key} is `None`.")

        if isinstance(val, (dict, list)):
            check_no_none(val)


def mcu_instruction(instruction: dict[str, Any]) -> dict[str, int]:
    """Parse instructions into a flat JSON for the Industruino IND.I/O."""
    indio_instruction = {}

    try:
        indio_instruction["mode"] = MODE_MAP[instruction["mode"]]
    except KeyError as err:
        raise InvalidMode("Mode provided is not valid.") from err

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
