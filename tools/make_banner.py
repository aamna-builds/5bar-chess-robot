#!/usr/bin/env python3
"""Draw media/banner.svg (README header) from the real v2 geometry."""
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "host"))
from fivebar_chess import board as B, config as C          # noqa: E402
from fivebar_chess.kinematics import elbows, ik              # noqa: E402

cfg = C.load()
W, H = 1280, 420
S = 0.62                                   # px per mm
OX, OY = 930, 380                          # screen position of the robot origin


def px(p):
    return OX + p[0] * S, OY - p[1] * S


out = []
a = out.append
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
a('<defs><linearGradient id="bg" x1="0" y1="0" x2="1" y2="1">'
  '<stop offset="0" stop-color="#0f172a"/><stop offset="1" stop-color="#1e293b"/></linearGradient>'
  '<radialGradient id="glow" cx="0.5" cy="0.5" r="0.5"><stop offset="0" stop-color="#f97316" stop-opacity="0.55"/>'
  '<stop offset="1" stop-color="#f97316" stop-opacity="0"/></radialGradient></defs>')
a(f'<rect width="{W}" height="{H}" rx="18" fill="url(#bg)"/>')

# subtle grid in the background
for x in range(0, W, 40):
    a(f'<line x1="{x}" y1="0" x2="{x}" y2="{H}" stroke="#ffffff" stroke-opacity="0.03"/>')
for y in range(0, H, 40):
    a(f'<line x1="0" y1="{y}" x2="{W}" y2="{y}" stroke="#ffffff" stroke-opacity="0.03"/>')

# chessboard
sq = cfg.square * S
for name in B.all_squares():
    x, y = px(B.square_xy(cfg, name))
    dark = ("abcdefgh".index(name[0]) + int(name[1]) - 1) % 2 == 0
    a(f'<rect x="{x - sq/2:.1f}" y="{y - sq/2:.1f}" width="{sq:.1f}" height="{sq:.1f}" '
      f'fill="{"#334155" if dark else "#475569"}"/>')
x0, y0 = px((-B.board_half(cfg), cfg.y0 + 8 * cfg.square))
a(f'<rect x="{x0:.1f}" y="{y0:.1f}" width="{8*sq:.1f}" height="{8*sq:.1f}" fill="none" '
  f'stroke="#94a3b8" stroke-width="1.5" rx="2"/>')

# a few pieces
glyph = dict(zip("KQRBNPkqrbnp", "♔♕♖♗♘♙♚♛♜♝♞♟"))
from fivebar_chess.chess import Position, sq_name   # noqa: E402
pos = Position.from_fen()
for m in "e2e4 e7e5 g1f3 b8c6 f1c4".split():          # Italian Game, black to play ...Nf6
    pos.push(m)
pieces = {sq_name(i): pc for i, pc in enumerate(pos.board) if pc and sq_name(i) != "g8"}
for s, pc in pieces.items():
    x, y = px(B.square_xy(cfg, s))
    col = "#f8fafc" if pc.isupper() else "#fb923c"
    stroke = ''
    a(f'<text x="{x:.1f}" y="{y + 7:.1f}" font-size="22" text-anchor="middle" fill="{col}"{stroke} '
      f'font-family="DejaVu Sans, Segoe UI Symbol, sans-serif">{glyph[pc]}</text>')

# knight path along the lanes g8 -> f6 (dashed)
h = cfg.square / 2
g8, f6 = B.square_xy(cfg, "g8"), B.square_xy(cfg, "f6")
lane = [g8, (g8[0] + h, g8[1] + h), (g8[0] + h, f6[1] - h), (f6[0] - h, f6[1] - h), f6]
pts = " ".join(f"{px(p)[0]:.1f},{px(p)[1]:.1f}" for p in lane)
a(f'<polyline points="{pts}" fill="none" stroke="#f97316" stroke-width="2.5" '
  f'stroke-dasharray="6 5" stroke-linecap="round" stroke-linejoin="round"/>')

# linkage, effector under f6
eff = f6
b1, b2 = elbows(cfg, ik(cfg, eff))
chain = [cfg.m1, b1, eff, b2, cfg.m2]
pts = " ".join(f"{px(p)[0]:.1f},{px(p)[1]:.1f}" for p in chain)
a(f'<polyline points="{pts}" fill="none" stroke="#e2e8f0" stroke-width="7" stroke-linejoin="round" '
  f'stroke-linecap="round" stroke-opacity="0.92"/>')
for p in (b1, b2):
    x, y = px(p)
    a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="7" fill="#0f172a" stroke="#e2e8f0" stroke-width="3"/>')
for p in (cfg.m1, cfg.m2):
    x, y = px(p)
    a(f'<rect x="{x-14:.1f}" y="{y-14:.1f}" width="28" height="28" rx="5" fill="#64748b" stroke="#e2e8f0" stroke-width="2"/>')
    a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="#e2e8f0"/>')
x, y = px(eff)
a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="34" fill="url(#glow)"/>')
a(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="9" fill="#f97316" stroke="#fff7ed" stroke-width="2.5"/>')
x, y = px(g8)
a(f'<text x="{x:.1f}" y="{y + 7:.1f}" font-size="22" text-anchor="middle" fill="#fb923c" '
  f'font-family="DejaVu Sans, Segoe UI Symbol, sans-serif" opacity="0.35">♞</text>')

# text block
F = "Segoe UI, Helvetica Neue, Helvetica, Arial, sans-serif"
a(f'<text x="70" y="150" font-family="{F}" font-size="58" font-weight="700" fill="#f8fafc">5-Bar Chess Robot</text>')
a(f'<text x="72" y="196" font-family="{F}" font-size="22" fill="#cbd5e1">A magnet under the board, driven by a</text>')
a(f'<text x="72" y="226" font-family="{F}" font-size="22" fill="#cbd5e1">2-DOF planar parallel linkage, moves real pieces.</text>')
tags = ["MATLAB", "Python", "Arduino", "Kinematics", "Path planning"]
x = 72
for t in tags:
    w = 18 + 10.5 * len(t)
    a(f'<rect x="{x}" y="262" width="{w:.0f}" height="34" rx="17" fill="#f97316" fill-opacity="0.14" stroke="#f97316" stroke-opacity="0.7"/>')
    a(f'<text x="{x + w/2:.0f}" y="285" font-family="{F}" font-size="16" text-anchor="middle" fill="#fdba74">{t}</text>')
    x += w + 10
a(f'<text x="72" y="350" font-family="{F}" font-size="15" fill="#64748b">L1 = {cfg.L1:g} mm · L2 = {cfg.L2:g} mm · '
  f'd = {cfg.d:g} mm · 400 × 400 mm board</text>')
a('</svg>')

dest = ROOT / "media" / "banner.svg"
dest.write_text("\n".join(out), encoding="utf-8")
print("wrote", dest.relative_to(ROOT))
