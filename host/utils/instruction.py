"""Instruction parsing module."""

from typing import Any, Final

MODES: Final[frozenset[str]] = frozenset(
    {
        "point_count",
        "point_time",
        "continuous",
    }
)


class InstructionFieldNone(Exception):
    """Exception for when a field of an instruction is `None`."""


def _ceil_div(num: int, denom: int) -> int:
    """Ceiling division between two integers."""
    return -(num // -denom)


def check_no_none(obj: Any) -> None:
    """Traverse all instruction fields and check that they are not `None`."""
    if isinstance(obj, dict):
        for key, val in obj.items():
            if val is None:
                raise InstructionFieldNone(f"'{key}' is None")
            check_no_none(val)

    elif isinstance(obj, list):
        for item in obj:
            check_no_none(item)


def mcu_instruction(instruction: dict[str, Any]) -> dict[str, int]:
    """Parse instructions into a flat JSON for the Industruino IND.I/O."""
    indio_instruction = {}

    mode = instruction["mode"]
    indio_instruction["mode"] = mode

    indio_instruction["x_min"] = instruction["grid"]["x"]["min"]
    indio_instruction["x_max"] = instruction["grid"]["x"]["max"]
    indio_instruction["x_unit_nm"] = instruction["grid"]["x"]["unit_nm"]
    indio_instruction["x_origin_nm"] = instruction["grid"]["x"]["origin_nm"]
    indio_instruction["y_min"] = instruction["grid"]["y"]["min"]
    indio_instruction["y_max"] = instruction["grid"]["y"]["max"]
    indio_instruction["y_unit_nm"] = instruction["grid"]["y"]["unit_nm"]
    indio_instruction["y_origin_nm"] = instruction["grid"]["y"]["origin_nm"]

    match mode:
        case "point_count":
            indio_instruction["num_pulses"] = instruction["capture"][
                "num_pulses"
            ]
            indio_instruction["posttrigger_time_us"] = _ceil_div(
                instruction["capture"]["posttrigger_time_ns"], 1000
            )
        case "point_time":
            indio_instruction["wait_time_us"] = instruction["capture"][
                "wait_time_us"
            ]

    check_no_none(indio_instruction)

    return indio_instruction
