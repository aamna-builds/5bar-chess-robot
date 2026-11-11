"""Physical layout: where every square and graveyard slot is, in mm."""
from __future__ import annotations

from .config import Config
from .kinematics import Point, is_safe

FILES = "abcdefgh"


def square_xy(cfg: Config, sq: str) -> Point:
    """Centre of a square such as 'e4'."""
    if len(sq) != 2 or sq[0] not in FILES or sq[1] not in "12345678":
        raise ValueError(f"not a square: {sq!r}")
    f, r = FILES.index(sq[0]), int(sq[1]) - 1
    col = 7 - f if cfg.a_side == "+x" else f
    row = 7 - r if cfg.rank_near == 8 else r
    return ((col - 3.5) * cfg.square, cfg.y0 + (row + 0.5) * cfg.square)


def all_squares() -> list[str]:
    return [f + r for r in "12345678" for f in FILES]


def board_half(cfg: Config) -> float:
    return 4 * cfg.square


def graveyard_slots(cfg: Config, white: bool) -> list[Point]:
    """Reachable parking slots for captured pieces of one colour, nearest the motors first."""
    side = 1.0 if (cfg.white_side == "+x") == white else -1.0
    x = side * (board_half(cfg) + cfg.grave_x_off)
    y = cfg.y0 + cfg.piece_dia / 2
    slots = []
    while y <= cfg.y0 + 8 * cfg.square - cfg.piece_dia / 2 + 1e-9:
        if is_safe(cfg, (x, y)):
            slots.append((x, y))
        y += cfg.grave_pitch
    return slots
