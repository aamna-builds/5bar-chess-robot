"""Command-line entry point:  python -m fivebar_chess <command> ...

Commands
--------
play     play a game: you type moves (or let an engine move) and the robot
         executes them. --sim runs without hardware.
replay   execute a list of UCI moves, e.g. from a file, and optionally save
         an animation (--gif) of what the robot did.
goto     drive the magnet to a square or graveyard slot, for calibration.
check    print kinematic quality for every square (sanity check of a config).
"""
from __future__ import annotations

import argparse
import sys

from . import board as B
from . import config as C
from .chess import Position
from .kinematics import quality
from .planner import Planner, PlanningError
from .robot import RobotError, SerialRobot, SimRobot, execute


def _robot(args, cfg):
    return SimRobot(cfg) if args.sim else SerialRobot(args.port)


def cmd_check(args) -> int:
    cfg = C.load(args.config)
    worst = None
    for r in "87654321":
        row = []
        for f in "abcdefgh":
            q = quality(cfg, B.square_xy(cfg, f + r))
            row.append(f"{q.margin_deg:5.1f}{' ' if q.safe else '!'}")
            if worst is None or q.margin_deg < worst[1]:
                worst = (f + r, q.margin_deg)
        print(r, " ".join(row))
    print("   " + "      ".join("abcdefgh"))
    print(f"worst square {worst[0]}: {worst[1]:.1f} deg from a singularity "
          f"(limits {cfg.min_trans:g} / {cfg.min_elbow:g} deg); '!' = outside limits")
    print(f"graveyard slots: white {len(B.graveyard_slots(cfg, True))}, black {len(B.graveyard_slots(cfg, False))}")
    return 0


def cmd_goto(args) -> int:
    cfg = C.load(args.config)
    robot = _robot(args, cfg)
    p = B.square_xy(cfg, args.square)
    robot.magnet(False)
    robot.move_to(p)
    print(f"magnet at {args.square} = ({p[0]:.1f}, {p[1]:.1f}) mm")
    return 0


def _run_moves(args, moves_iter, cfg, robot, pos, planner) -> int:
    for uci in moves_iter:
        try:
            plan, info = planner.plan(pos, uci)
        except ValueError as e:
            print(f"  {e}")
            if args.strict:
                return 1
            continue
        execute(robot, plan)
        desc = ", ".join(t.label for t in plan.transfers)
        print(f"{plan.move:6s} {desc}")
        for n in plan.notes:
            print("  NOTE:", n)
        if pos.outcome():
            print("game over:", pos.outcome())
            break
    return 0


def cmd_replay(args) -> int:
    cfg = C.load(args.config)
    robot = _robot(args, cfg)
    pos = Position.from_fen(args.fen) if args.fen else Position.from_fen()
    start = pos.copy()
    text = open(args.moves).read() if args.moves != "-" else sys.stdin.read()
    moves = [m for m in text.replace(",", " ").split() if not m.endswith(".")]
    rc = _run_moves(args, moves, cfg, robot, pos, Planner(cfg))
    if args.gif:
        if not isinstance(robot, SimRobot):
            print("--gif needs --sim"); return 2
        from .sim_view import animate
        animate(cfg, robot, args.gif, start=start, title=args.title or "")
        print("wrote", args.gif)
    return rc


def cmd_play(args) -> int:
    cfg = C.load(args.config)
    robot = _robot(args, cfg)
    pos = Position.from_fen()
    planner = Planner(cfg)
    engine = None
    if args.engine:
        from .engine import UciEngine
        engine = UciEngine(args.engine, skill=args.skill)
    human_white = args.colour == "white"
    print(pos)
    while not pos.outcome():
        if engine and pos.white_to_move != human_white:
            mv = engine.best_move(pos.fen(), args.movetime)
            print(f"engine: {mv}")
        else:
            mv = input("your move (UCI, e.g. e2e4; 'q' quits): ").strip()
            if mv in ("q", "quit", "exit"):
                break
        try:
            plan, _ = planner.plan(pos, mv)
        except (ValueError, PlanningError) as e:
            print(" ", e)
            continue
        try:
            execute(robot, plan)
        except RobotError as e:
            print("robot error:", e)
            return 1
        for n in plan.notes:
            print("NOTE:", n)
        print(pos)
    print("result:", pos.outcome() or "stopped")
    if engine:
        engine.close()
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="fivebar_chess", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--config", help="robot JSON (default config/robot.json)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    def hw(p):
        g = p.add_mutually_exclusive_group(required=True)
        g.add_argument("--sim", action="store_true", help="no hardware, simulate")
        g.add_argument("--port", help="serial port, e.g. /dev/ttyUSB0 or COM3")

    p = sub.add_parser("check", help="kinematic quality of every square"); p.set_defaults(fn=cmd_check)
    p = sub.add_parser("goto", help="move the magnet to a square"); hw(p)
    p.add_argument("square"); p.set_defaults(fn=cmd_goto)
    p = sub.add_parser("replay", help="execute a list of UCI moves"); hw(p)
    p.add_argument("moves", help="file with UCI moves, or - for stdin")
    p.add_argument("--fen"); p.add_argument("--gif"); p.add_argument("--title")
    p.add_argument("--strict", action="store_true", help="stop at the first illegal move")
    p.set_defaults(fn=cmd_replay)
    p = sub.add_parser("play", help="interactive game"); hw(p)
    p.add_argument("--engine", help="path to a UCI engine executable (optional)")
    p.add_argument("--skill", type=int, default=5)
    p.add_argument("--movetime", type=int, default=500)
    p.add_argument("--colour", choices=["white", "black"], default="white")
    p.set_defaults(fn=cmd_play)
    args = ap.parse_args(argv)
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
