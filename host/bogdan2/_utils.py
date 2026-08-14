"""Miscellaneous utility functions."""


def ceil_div(num: int, denom: int) -> int:
    """Ceiling division between two integers."""
    return -(num // -denom)
