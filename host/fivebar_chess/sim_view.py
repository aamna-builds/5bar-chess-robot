"""Animate a SimRobot log with matplotlib (offline preview of a game)."""
from __future__ import annotations

import math

from . import board as B
from .chess import Position, sq_name
from .config import Config
from .kinematics import elbows, ik
from .robot import SimRobot


def _densify(log, step_mm: float):
    """Expand the waypoint log into evenly spaced frames."""
    frames = []
    prev = log[0][1]
    for kind, p, mag in log:
        if kind == "move" or kind == "home":
            n = max(1, math.ceil(math.dist(prev, p) / step_mm))
            for i in range(1, n + 1):
                t = i / n
                frames.append(((prev[0] + t * (p[0] - prev[0]), prev[1] + t * (p[1] - prev[1])), mag))
            prev = p
        else:
            frames.extend([(p, mag)] * 3)    # short pause for the magnet servo
    return frames


GLYPH = dict(zip("KQRBNPkqrbnp", "\u2654\u2655\u2656\u2657\u2658\u2659\u265a\u265b\u265c\u265d\u265e\u265f"))


def animate(cfg: Config, robot: SimRobot, out_file: str, start: Position | None = None,
            step_mm: float = 12.0, fps: int = 20, title: str = "") -> None:
    """Write an animated GIF of everything the simulated robot did.

    If ``start`` is given, the pieces are drawn and slide with the magnet."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation, PillowWriter

    frames = _densify(robot.log, step_mm)
    fig, ax = plt.subplots(figsize=(5, 5.2), dpi=90)
    half, s = B.board_half(cfg), cfg.square
    for name in B.all_squares():
        x, y = B.square_xy(cfg, name)
        dark = ("abcdefgh".index(name[0]) + int(name[1]) - 1) % 2 == 0     # a1 is dark
        ax.add_patch(plt.Rectangle((x - s / 2, y - s / 2), s, s,
                                   color="#769656" if dark else "#eeeed2", zorder=0))
    pieces = []                       # [x, y, letter, text artist]
    if start is not None:
        for i, pc in enumerate(start.board):
            if pc:
                x, y = B.square_xy(cfg, sq_name(i))
                t = ax.text(x, y, GLYPH[pc], ha="center", va="center", fontsize=17, zorder=5,
                            color="black")
                pieces.append([x, y, pc, t])
    carried = [None]
    (link,) = ax.plot([], [], "-o", color="#333", lw=3, mfc="w", zorder=3)
    (eff,) = ax.plot([], [], "o", ms=11, zorder=4)
    trail_x, trail_y = [], []
    (trail,) = ax.plot([], [], "-", color="#d95f02", lw=1.2, alpha=0.7, zorder=2)
    ax.plot([cfg.m1[0], cfg.m2[0]], [0, 0], "s", ms=12, color="#444")
    ax.set_xlim(-half - 90, half + 90)
    ax.set_ylim(-60, cfg.y0 + 8 * s + 40)
    ax.set_aspect("equal")
    ax.set_xticks([]); ax.set_yticks([])
    ax.set_title(title, fontsize=10)

    def update(i):
        p, mag = frames[i]
        th = ik(cfg, p)
        b1, b2 = elbows(cfg, th)
        pts = [cfg.m1, b1, p, b2, cfg.m2]
        link.set_data([q[0] for q in pts], [q[1] for q in pts])
        eff.set_data([p[0]], [p[1]])
        eff.set_color("#d62728" if mag else "#999999")
        if mag:
            trail_x.append(p[0]); trail_y.append(p[1])
        else:
            trail_x.append(math.nan); trail_y.append(math.nan)
        trail.set_data(trail_x, trail_y)
        if mag and carried[0] is None:
            near = [q for q in pieces if math.dist((q[0], q[1]), p) < 5]
            carried[0] = near[0] if near else None
        if not mag:
            carried[0] = None
        if carried[0] is not None:
            carried[0][0], carried[0][1] = p
            carried[0][3].set_position(p)
        return [link, eff, trail] + [q[3] for q in pieces]

    anim = FuncAnimation(fig, update, frames=len(frames), blit=True)
    anim.save(out_file, writer=PillowWriter(fps=fps))
    plt.close(fig)
