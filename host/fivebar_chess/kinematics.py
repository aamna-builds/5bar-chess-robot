"""Kinematics of the 5-bar linkage. Same maths and frame as matlab/fb_*.m.

Frame: origin midway between the motor shafts, +x right, +y towards the board.
Working mode: elbows out (left elbow swings left, right elbow swings right).
Assembly mode: effector on the far side of the elbows.
"""
from __future__ import annotations

import math
from dataclasses import dataclass

from .config import Config

Point = tuple[float, float]


class Unreachable(ValueError):
    pass


def ik(cfg: Config, p: Point) -> tuple[float, float]:
    """Motor angles (rad, CCW from +x) that put the effector at p."""
    x, y = p
    out = []
    for (mx, my), sign in ((cfg.m1, +1.0), (cfg.m2, -1.0)):
        dx, dy = x - mx, y - my
        r = math.hypot(dx, dy)
        if r < 1e-9:
            raise Unreachable(f"{p} is on a motor axis")
        c = (cfg.L1**2 + r**2 - cfg.L2**2) / (2 * cfg.L1 * r)
        if abs(c) > 1:
            raise Unreachable(f"({x:.1f}, {y:.1f}) is out of reach")
        out.append(math.atan2(dy, dx) + sign * math.acos(c))
    return out[0], out[1]


def elbows(cfg: Config, th: tuple[float, float]) -> tuple[Point, Point]:
    (m1x, m1y), (m2x, m2y) = cfg.m1, cfg.m2
    b1 = (m1x + cfg.L1 * math.cos(th[0]), m1y + cfg.L1 * math.sin(th[0]))
    b2 = (m2x + cfg.L1 * math.cos(th[1]), m2y + cfg.L1 * math.sin(th[1]))
    return b1, b2


def fk(cfg: Config, th: tuple[float, float]) -> Point:
    b1, b2 = elbows(cfg, th)
    vx, vy = b2[0] - b1[0], b2[1] - b1[1]
    dist = math.hypot(vx, vy)
    if dist < 1e-9 or dist > 2 * cfg.L2:
        raise Unreachable("linkage cannot be assembled at these angles")
    h = math.sqrt(max(0.0, cfg.L2**2 - (dist / 2) ** 2))
    ux, uy = vx / dist, vy / dist
    return ((b1[0] + b2[0]) / 2 - h * uy, (b1[1] + b2[1]) / 2 + h * ux)


def _angle(u: Point, v: Point) -> float:
    cross = u[0] * v[1] - u[1] * v[0]
    return math.degrees(math.atan2(abs(cross), u[0] * v[0] + u[1] * v[1]))


@dataclass(frozen=True)
class PoseQuality:
    trans_deg: float     # angle between distal links at the effector
    elbow_deg: float     # smaller elbow angle
    margin_deg: float    # distance to the nearest singularity
    safe: bool


def quality(cfg: Config, p: Point) -> PoseQuality:
    try:
        th = ik(cfg, p)
    except Unreachable:
        return PoseQuality(math.nan, math.nan, -math.inf, False)
    b1, b2 = elbows(cfg, th)
    trans = _angle((b1[0] - p[0], b1[1] - p[1]), (b2[0] - p[0], b2[1] - p[1]))
    e1 = _angle((cfg.m1[0] - b1[0], cfg.m1[1] - b1[1]), (p[0] - b1[0], p[1] - b1[1]))
    e2 = _angle((cfg.m2[0] - b2[0], cfg.m2[1] - b2[1]), (p[0] - b2[0], p[1] - b2[1]))
    elbow = min(e1, e2)
    tm, em = min(trans, 180 - trans), min(elbow, 180 - elbow)
    return PoseQuality(trans, elbow, min(tm, em), tm >= cfg.min_trans and em >= cfg.min_elbow)


def is_safe(cfg: Config, p: Point) -> bool:
    return quality(cfg, p).safe


def steps_from_home(cfg: Config, p: Point) -> tuple[int, int]:
    th = ik(cfg, p)
    return tuple(  # type: ignore[return-value]
        round((math.degrees(t) - h) / cfg.deg_per_step) for t, h in zip(th, cfg.home_deg)
    )
