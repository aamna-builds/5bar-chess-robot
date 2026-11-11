"""Turn chess moves into magnet paths the robot can execute.

A chess move becomes one or more *transfers* (pick a piece up magnetically
at A, slide it to B). Each transfer becomes a list of waypoints:

* straight line A -> B when it keeps at least ``cfg.clearance`` from every
  other piece (rook/bishop/queen/king/pawn moves on an open board, diagonals
  pass between pieces through the square corners);
* otherwise along the *lanes*, the grid lines between squares. Pieces are
  centred in their squares, so a lane passes every piece at half a square,
  which is more than the clearance for the configured piece size.

Every sampled point of every path is checked against the kinematic safety
limits before the plan is accepted.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field

from . import board as B
from .chess import MoveInfo, Position, is_white, sq_name
from .config import Config
from .kinematics import Point, is_safe


class PlanningError(RuntimeError):
    pass


@dataclass
class Transfer:
    """Slide one piece from one place to another with the magnet engaged."""
    piece: str
    label: str
    waypoints: list[Point]

    @property
    def length(self) -> float:
        return path_length(self.waypoints)


@dataclass
class Plan:
    move: str
    transfers: list[Transfer] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def path_length(w: list[Point]) -> float:
    return sum(math.dist(a, b) for a, b in zip(w, w[1:]))


def _seg_point_dist(p: Point, a: Point, b: Point) -> float:
    ax, ay = a
    dx, dy = b[0] - ax, b[1] - ay
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / L2))
    return math.dist(p, (ax + t * dx, ay + t * dy))


def path_clear(w: list[Point], obstacles: list[Point], clearance: float) -> bool:
    return all(_seg_point_dist(o, a, b) >= clearance - 1e-9
               for a, b in zip(w, w[1:]) for o in obstacles)


def path_safe(cfg: Config, w: list[Point], step: float = 2.0) -> bool:
    for a, b in zip(w, w[1:]):
        n = max(1, math.ceil(math.dist(a, b) / step))
        for i in range(n + 1):
            t = i / n
            if not is_safe(cfg, (a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]))):
                return False
    return True


def _dedupe(w: list[Point]) -> list[Point]:
    out = [w[0]]
    for p in w[1:]:
        if math.dist(p, out[-1]) > 1e-9:
            out.append(p)
    # drop collinear middle points
    i = 1
    while i < len(out) - 1:
        (ax, ay), (bx, by), (cx, cy) = out[i - 1], out[i], out[i + 1]
        if abs((bx - ax) * (cy - ay) - (by - ay) * (cx - ax)) < 1e-9:
            out.pop(i)
        else:
            i += 1
    return out


def lane_candidates(cfg: Config, a: Point, b: Point) -> list[list[Point]]:
    """Routes from a to b that use the grid lines between squares."""
    h = cfg.square / 2
    routes = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            c1 = (a[0] + sx * h, a[1] + sy * h)              # a corner of the start square
            for tx in (-1, 1):
                for ty in (-1, 1):
                    c2 = (b[0] + tx * h, b[1] + ty * h)      # a corner of the target square
                    for mid in ((c2[0], c1[1]), (c1[0], c2[1])):
                        routes.append(_dedupe([a, c1, mid, c2, b]))
    # routes that run along the lane at the board edge (to/from the graveyard)
    half = B.board_half(cfg)
    ends_a = [a] + [(a[0] + sx * h, a[1] + sy * h) for sx in (-1, 1) for sy in (-1, 1)]
    ends_b = [b] + [(b[0] + sx * h, b[1] + sy * h) for sx in (-1, 1) for sy in (-1, 1)]
    for ex in (-half, half):
        for c1 in ends_a:
            for c2 in ends_b:
                routes.append(_dedupe([a, c1, (ex, c1[1]), (ex, c2[1]), c2, b]))
    return routes


def route(cfg: Config, a: Point, b: Point, obstacles: list[Point]) -> list[Point]:
    """Shortest collision-free, kinematically safe path from a to b."""
    direct = [a, b]
    if path_clear(direct, obstacles, cfg.clearance) and path_safe(cfg, direct):
        return direct
    best = None
    for w in sorted(lane_candidates(cfg, a, b), key=path_length):
        if path_clear(w, obstacles, cfg.clearance) and path_safe(cfg, w):
            best = w
            break
    if best is None:
        raise PlanningError(f"no safe path from {a} to {b}")
    return best


class Planner:
    """Keeps track of where every physical piece is, including the graveyard."""

    def __init__(self, cfg: Config):
        self.cfg = cfg
        self.slots = {True: B.graveyard_slots(cfg, white=True),
                      False: B.graveyard_slots(cfg, white=False)}
        self.parked: dict[Point, str] = {}       # graveyard slot -> piece letter

    # -- helpers -------------------------------------------------------------
    def _obstacles(self, pos: Position, ignore: set[int] = frozenset()) -> list[Point]:
        pts = [B.square_xy(self.cfg, sq_name(i))
               for i, p in enumerate(pos.board) if p is not None and i not in ignore]
        return pts + list(self.parked)

    def _free_slot(self, white: bool) -> Point:
        for s in self.slots[white]:
            if s not in self.parked:
                return s
        raise PlanningError(f"graveyard for {'white' if white else 'black'} is full")

    def _to_graveyard(self, pos: Position, sq: int, piece: str) -> Transfer:
        a = B.square_xy(self.cfg, sq_name(sq))
        slot = self._free_slot(is_white(piece))
        obstacles = self._obstacles(pos, ignore={sq})
        w = route(self.cfg, a, slot, obstacles)
        self.parked[slot] = piece
        return Transfer(piece, f"{piece} {sq_name(sq)} -> graveyard", w)

    def _from_graveyard(self, pos: Position, piece: str, sq: int) -> Transfer | None:
        for slot, p in self.parked.items():
            if p == piece:
                del self.parked[slot]
                obstacles = self._obstacles(pos, ignore={sq})
                w = route(self.cfg, slot, B.square_xy(self.cfg, sq_name(sq)), obstacles)
                return Transfer(piece, f"{piece} graveyard -> {sq_name(sq)}", w)
        return None

    # -- main entry ----------------------------------------------------------
    def plan(self, pos: Position, uci: str) -> tuple[Plan, MoveInfo]:
        """Plan the physical execution of ``uci`` and play it on ``pos``."""
        before = pos.copy()
        info = pos.push(uci)                     # raises on illegal moves
        m = info.move
        plan = Plan(move=m.uci())
        work = before                            # occupancy evolves as we go
        cfg = self.cfg

        if info.captured_on is not None:
            plan.transfers.append(self._to_graveyard(work, info.captured_on, info.captured))
            work.board[info.captured_on] = None

        a = B.square_xy(cfg, sq_name(m.frm))
        b = B.square_xy(cfg, sq_name(m.to))
        w = route(cfg, a, b, self._obstacles(work, ignore={m.frm}))
        plan.transfers.append(Transfer(info.piece, f"{info.piece} {sq_name(m.frm)}-{sq_name(m.to)}", w))
        work.board[m.to], work.board[m.frm] = info.piece, None

        if info.rook_from is not None:
            rook = work.board[info.rook_from]
            ra = B.square_xy(cfg, sq_name(info.rook_from))
            rb = B.square_xy(cfg, sq_name(info.rook_to))
            w = route(cfg, ra, rb, self._obstacles(work, ignore={info.rook_from}))
            plan.transfers.append(Transfer(rook, f"{rook} {sq_name(info.rook_from)}-{sq_name(info.rook_to)} (castling)", w))
            work.board[info.rook_to], work.board[info.rook_from] = rook, None

        if info.promoted_to:
            plan.transfers.append(self._to_graveyard(work, m.to, info.piece))
            work.board[m.to] = None
            fetch = self._from_graveyard(work, info.promoted_to, m.to)
            if fetch:
                plan.transfers.append(fetch)
            else:
                plan.notes.append(f"Place a {info.promoted_to} on {sq_name(m.to)} by hand (none in the graveyard).")
            work.board[m.to] = info.promoted_to
        return plan, info
