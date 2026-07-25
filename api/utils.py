"""Utility functions for host-side operations."""


def ceil_div(num: int, denom: int) -> int:
    """Ceiling division between two integers."""
    return -(num // -denom)
