import math
import random
from pathlib import Path

import pytest

from fivebar_chess import board as B
from fivebar_chess import config as C
from fivebar_chess.chess import Position, sq_name
from fivebar_chess.planner import Planner, path_clear, path_safe
from fivebar_chess.robot import SimRobot, execute

EXAMPLES = Path(__file__).resolve().parents[1] / "examples"


@pytest.fixture()
def cfg():
    return C.load()


def _check_plan(cfg, planner, before, plan):
    """Re-verify every transfer against the occupancy at the time it runs."""
    occ = {B.square_xy(cfg, sq_name(i)) for i, p in enumerate(before.board) if p}
    occ |= set(planner.parked)       # pieces parked after the plan (superset is fine)
    for t in plan.transfers:
        start = t.waypoints[0]
        others = [o for o in occ if math.dist(o, start) > 1e-6 and math.dist(o, t.waypoints[-1]) > 1e-6]
        assert path_clear(t.waypoints, others, cfg.clearance), t.label
        assert path_safe(cfg, t.waypoints), t.label
        occ.discard(start)
        occ.add(t.waypoints[-1])


@pytest.mark.parametrize("name", ["italian_game.txt", "special_moves.txt"])
def test_example_games(cfg, name):
    pos, planner, robot = Position.from_fen(), Planner(cfg), SimRobot(cfg)
    for mv in (EXAMPLES / name).read_text().split():
        before = pos.copy()
        plan, _ = planner.plan(pos, mv)
        _check_plan(cfg, planner, before, plan)
        execute(robot, plan)


def test_random_games(cfg):
    rng = random.Random(2026)
    for _ in range(6):
        pos, planner = Position.from_fen(), Planner(cfg)
        for _ in range(120):
            moves = pos.legal_moves()
            if not moves or pos.outcome():
                break
            mv = rng.choice(moves)
            before = pos.copy()
            plan, _ = planner.plan(pos, mv.uci())
            _check_plan(cfg, planner, before, plan)


def test_castling_moves_rook(cfg):
    pos = Position.from_fen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1")
    plan, _ = Planner(cfg).plan(pos, "e1g1")
    assert [t.piece for t in plan.transfers] == ["K", "R"]
    assert plan.transfers[-1].waypoints[-1] == B.square_xy(cfg, "f1")


def test_en_passant_removes_the_right_pawn(cfg):
    pos = Position.from_fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1")
    plan, _ = Planner(cfg).plan(pos, "e5d6")
    assert plan.transfers[0].waypoints[0] == B.square_xy(cfg, "d5")


def test_promotion_fetches_captured_queen(cfg):
    pos = Position.from_fen("3qk3/P7/8/8/8/8/8/3QK3 w - - 0 1")
    planner = Planner(cfg)
    planner.plan(pos, "d1d8")          # Qxd8+ : black queen parked
    planner.plan(pos, "e8d8")          # Kxd8  : white queen parked
    plan, _ = planner.plan(pos, "a7a8q")
    labels = [t.label for t in plan.transfers]
    assert labels[-1].startswith("Q graveyard -> a8"), labels
    assert not plan.notes


def test_graveyard_fills_without_collisions(cfg):
    """Park all 15 black non-king pieces from the start position, one by one."""
    planner = Planner(cfg)
    pos = Position.from_fen()
    order = "a7 b7 c7 d7 e7 f7 g7 h7 a8 b8 c8 d8 f8 g8 h8".split()
    for name in order:
        i = B.FILES.index(name[0]) + 8 * (int(name[1]) - 1)
        before_parked = set(planner.parked)
        t = planner._to_graveyard(pos, i, pos.board[i])
        others = [B.square_xy(cfg, sq_name(j)) for j, p in enumerate(pos.board) if p and j != i]
        assert path_clear(t.waypoints, others + list(before_parked), cfg.clearance), name
        assert path_safe(cfg, t.waypoints), name
        pos.board[i] = None
    assert len(planner.parked) == 15
    assert all(x < 0 for x, _ in planner.parked)          # black pieces go to the -x side
