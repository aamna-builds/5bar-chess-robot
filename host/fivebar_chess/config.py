"""Load the shared robot configuration (config/robot.json)."""
from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = REPO_ROOT / "config" / "robot.json"


@dataclass(frozen=True)
class Config:
    L1: float              # proximal link, motor -> elbow [mm]
    L2: float              # distal link, elbow -> effector [mm]
    d: float               # motor shaft spacing [mm]
    square: float          # chess square size [mm]
    y0: float              # board edge nearest the motors [mm]
    rank_near: int         # rank closest to the motors (1 or 8)
    a_side: str            # "+x" or "-x"
    piece_dia: float       # piece base diameter [mm]
    grave_x_off: float     # graveyard column distance from board edge [mm]
    grave_pitch: float     # spacing of graveyard slots [mm]
    white_side: str        # graveyard side for captured white pieces
    steps_per_rev: float   # motor-shaft steps incl. microsteps and gearbox
    home_deg: tuple[float, float]
    vmax: float
    amax: float
    min_trans: float
    min_elbow: float
    magnet_settle_ms: int
    raw: dict

    @property
    def deg_per_step(self) -> float:
        return 360.0 / self.steps_per_rev

    @property
    def clearance(self) -> float:
        """Minimum distance between a carried piece and any other piece centre."""
        return self.piece_dia + 2.0

    @property
    def m1(self) -> tuple[float, float]:
        return (-self.d / 2.0, 0.0)

    @property
    def m2(self) -> tuple[float, float]:
        return (self.d / 2.0, 0.0)


def load(path: str | Path | None = None) -> Config:
    p = Path(path) if path else DEFAULT_CONFIG
    c = json.loads(p.read_text())
    g, b, dr, m, s = c["geometry"], c["board"], c["drive"], c["motion"], c["safety"]
    steps = dr["full_steps_per_rev"] * dr["microsteps"] * dr["gear_ratio"]
    cfg = Config(
        L1=g["proximal_mm"], L2=g["distal_mm"], d=g["motor_spacing_mm"],
        square=b["square_mm"], y0=b["near_edge_y_mm"], rank_near=b["rank_nearest_motors"],
        a_side=b["a_file_side"], piece_dia=c["piece"]["base_diameter_mm"],
        grave_x_off=c["graveyard"]["x_offset_from_board_edge_mm"],
        grave_pitch=c["graveyard"]["pitch_mm"], white_side=c["graveyard"]["white_side"],
        steps_per_rev=steps, home_deg=tuple(dr["home_deg"]),
        vmax=m["max_speed_mm_s"], amax=m["accel_mm_s2"],
        min_trans=s["min_transmission_deg"], min_elbow=s["min_elbow_deg"],
        magnet_settle_ms=m["magnet_settle_ms"], raw=c,
    )
    if cfg.rank_near not in (1, 8) or cfg.a_side not in ("+x", "-x"):
        raise ValueError("board orientation in config is invalid")
    if not math.isfinite(steps) or steps <= 0:
        raise ValueError("drive settings in config are invalid")
    return cfg
