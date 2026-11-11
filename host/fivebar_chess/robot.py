"""Talk to the firmware, or simulate it.

Protocol (one ASCII line per command, the firmware answers one line):

    G <x> <y>        move the magnet in a straight line to (x, y) mm -> "ok" when finished
    F <mm/s>         set travel speed                                 -> "ok"
    M1 / M0          engage / release the magnet                      -> "ok"
    Z                current pose is the home pose (both arms at home_deg) -> "ok"
    H                travel back to the home pose                     -> "ok"
    ?                report "pos <x> <y> mag <0|1>"
    E1 / E0          enable / disable the motor drivers               -> "ok"

Errors come back as "err <reason>" and the robot does not move.
"""
from __future__ import annotations

import math
import time
from dataclasses import dataclass, field
from typing import Protocol

from .config import Config
from .kinematics import Point, fk, is_safe
from .planner import Plan


class RobotError(RuntimeError):
    pass


class Robot(Protocol):
    def move_to(self, p: Point) -> None: ...
    def magnet(self, on: bool) -> None: ...
    def home(self) -> None: ...


def home_point(cfg: Config) -> Point:
    return fk(cfg, tuple(math.radians(a) for a in cfg.home_deg))  # type: ignore[arg-type]


@dataclass
class SimRobot:
    """Records every motion so it can be checked or animated (see sim_view)."""
    cfg: Config
    pos: Point = (0.0, 0.0)
    mag: bool = False
    log: list[tuple[str, Point, bool]] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.pos = home_point(self.cfg)
        self.log.append(("home", self.pos, False))

    def move_to(self, p: Point) -> None:
        if not is_safe(self.cfg, p):
            raise RobotError(f"err unsafe {p[0]:.1f} {p[1]:.1f}")
        self.pos = p
        self.log.append(("move", p, self.mag))

    def magnet(self, on: bool) -> None:
        self.mag = on
        self.log.append(("magnet", self.pos, on))

    def home(self) -> None:
        self.pos = home_point(self.cfg)
        self.log.append(("home", self.pos, self.mag))


class SerialRobot:
    """Drives the real robot over USB serial (needs ``pip install pyserial``)."""

    def __init__(self, port: str, baud: int = 115200, timeout: float = 30.0):
        try:
            import serial  # type: ignore
        except ImportError as e:  # pragma: no cover - depends on the machine
            raise RobotError("pyserial is not installed: pip install pyserial") from e
        self.ser = serial.Serial(port, baud, timeout=timeout)
        time.sleep(2.0)                     # Uno resets when the port opens
        self.ser.reset_input_buffer()
        banner = self.cmd("?")
        if not banner.startswith("pos"):
            raise RobotError(f"unexpected reply from firmware: {banner!r}")

    def cmd(self, line: str) -> str:
        self.ser.write((line + "\n").encode())
        reply = self.ser.readline().decode(errors="replace").strip()
        if not reply:
            raise RobotError(f"no reply to {line!r} (timeout)")
        if reply.startswith("err"):
            raise RobotError(f"{line!r} -> {reply}")
        return reply

    def move_to(self, p: Point) -> None:
        self.cmd(f"G {p[0]:.2f} {p[1]:.2f}")

    def magnet(self, on: bool) -> None:
        self.cmd("M1" if on else "M0")

    def home(self) -> None:
        self.cmd("H")

    def close(self) -> None:
        self.ser.close()


def execute(robot: Robot, plan: Plan, settle_s: float = 0.0) -> None:
    """Run a plan: travel with the magnet off, carry with it on."""
    for t in plan.transfers:
        robot.magnet(False)
        robot.move_to(t.waypoints[0])
        robot.magnet(True)
        if settle_s:
            time.sleep(settle_s)
        for p in t.waypoints[1:]:
            robot.move_to(p)
        robot.magnet(False)
