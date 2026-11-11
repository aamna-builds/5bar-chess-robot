import csv
import math
from pathlib import Path

import pytest

from fivebar_chess import board as B
from fivebar_chess import config as C
from fivebar_chess.kinematics import Unreachable, fk, ik, quality, steps_from_home

ROOT = Path(__file__).resolve().parents[2]
TABLE = ROOT / "matlab" / "output" / "square_table.csv"


@pytest.fixture(scope="module")
def cfg():
    return C.load()


def test_round_trip_over_board(cfg):
    for sq in B.all_squares():
        p = B.square_xy(cfg, sq)
        q = fk(cfg, ik(cfg, p))
        assert math.dist(p, q) < 1e-9


def test_every_square_and_slot_is_safe(cfg):
    for sq in B.all_squares():
        assert quality(cfg, B.square_xy(cfg, sq)).safe, sq
    for white in (True, False):
        assert len(B.graveyard_slots(cfg, white)) >= 15


def test_out_of_reach_raises(cfg):
    with pytest.raises(Unreachable):
        ik(cfg, (0.0, cfg.L1 + cfg.L2 + 50))


def test_board_orientation(cfg):
    a1, h8 = B.square_xy(cfg, "a1"), B.square_xy(cfg, "h8")
    assert a1[0] > 0 and a1[1] > h8[1]          # a-file on +x, rank 8 nearest the motors
    assert math.isclose(math.dist(B.square_xy(cfg, "e4"), B.square_xy(cfg, "e5")), cfg.square)


@pytest.mark.skipif(not TABLE.exists(), reason="run matlab/run_all.m first")
def test_matches_matlab_table(cfg):
    """Python and MATLAB must agree on every square (same config, two implementations)."""
    rows = {r["name"]: r for r in csv.DictReader(TABLE.open())}
    for sq in B.all_squares():
        r = rows[sq]
        x, y = B.square_xy(cfg, sq)
        assert math.isclose(x, float(r["x_mm"]), abs_tol=0.01)
        assert math.isclose(y, float(r["y_mm"]), abs_tol=0.01)
        th = [math.degrees(t) for t in ik(cfg, (x, y))]
        assert math.isclose(th[0], float(r["left_deg"]), abs_tol=0.002)
        assert math.isclose(th[1], float(r["right_deg"]), abs_tol=0.002)
        s = steps_from_home(cfg, (x, y))
        assert abs(s[0] - int(r["left_steps"])) <= 1 and abs(s[1] - int(r["right_steps"])) <= 1


def test_home_pose_is_safe(cfg):
    from fivebar_chess.robot import home_point
    assert quality(cfg, home_point(cfg)).safe
