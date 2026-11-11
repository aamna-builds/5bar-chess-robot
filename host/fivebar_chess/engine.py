"""Minimal UCI client for an external chess engine such as Stockfish.

The engine is a separate program that you install yourself; nothing from it
is included in this repository. Example: ``--engine /usr/games/stockfish``.
"""
from __future__ import annotations

import subprocess


class UciEngine:
    def __init__(self, path: str, skill: int | None = None):
        self.p = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1)
        self._send("uci")
        self._wait("uciok")
        if skill is not None:
            self._send(f"setoption name Skill Level value {int(skill)}")
        self._send("isready")
        self._wait("readyok")

    def _send(self, line: str) -> None:
        assert self.p.stdin is not None
        self.p.stdin.write(line + "\n")
        self.p.stdin.flush()

    def _wait(self, token: str) -> str:
        assert self.p.stdout is not None
        while True:
            line = self.p.stdout.readline()
            if not line:
                raise RuntimeError("engine exited unexpectedly")
            if line.startswith(token):
                return line.strip()

    def best_move(self, fen: str, movetime_ms: int = 500) -> str:
        self._send(f"position fen {fen}")
        self._send(f"go movetime {movetime_ms}")
        return self._wait("bestmove").split()[1]

    def close(self) -> None:
        try:
            self._send("quit")
            self.p.wait(timeout=2)
        except Exception:
            self.p.kill()
