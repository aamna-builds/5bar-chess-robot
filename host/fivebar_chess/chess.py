"""A compact chess rules engine written for this project.

It knows every rule the robot has to execute physically: captures, castling,
en passant, promotion, check, checkmate, stalemate and the fifty-move rule.
It is deliberately small and dependency-free; it is verified with standard
perft node counts in tests/test_chess.py.

Squares are indexed 0..63 with a1 = 0, b1 = 1, ..., h8 = 63.
Pieces are FEN letters: upper case = white, lower case = black.
"""
from __future__ import annotations

from dataclasses import dataclass, field

FILES = "abcdefgh"
START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

KNIGHT = [(1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)]
KING = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
ROOK_DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1)]
BISHOP_DIRS = [(1, 1), (1, -1), (-1, 1), (-1, -1)]


def sq_name(i: int) -> str:
    return FILES[i % 8] + str(i // 8 + 1)


def sq_index(name: str) -> int:
    if len(name) != 2 or name[0] not in FILES or name[1] not in "12345678":
        raise ValueError(f"not a square: {name!r}")
    return FILES.index(name[0]) + 8 * (int(name[1]) - 1)


def _on(f: int, r: int) -> bool:
    return 0 <= f < 8 and 0 <= r < 8


def is_white(p: str) -> bool:
    return p.isupper()


@dataclass(frozen=True)
class Move:
    frm: int
    to: int
    promo: str | None = None          # 'q', 'r', 'b' or 'n' (lower case)

    @classmethod
    def from_uci(cls, s: str) -> "Move":
        s = s.strip().lower()
        if len(s) not in (4, 5):
            raise ValueError(f"not a UCI move: {s!r}")
        promo = s[4] if len(s) == 5 else None
        if promo is not None and promo not in "qrbn":
            raise ValueError(f"bad promotion piece in {s!r}")
        return cls(sq_index(s[:2]), sq_index(s[2:4]), promo)

    def uci(self) -> str:
        return sq_name(self.frm) + sq_name(self.to) + (self.promo or "")

    def __str__(self) -> str:
        return self.uci()


@dataclass
class MoveInfo:
    """What a move does on the physical board."""
    move: Move
    piece: str
    captured: str | None = None
    captured_on: int | None = None      # differs from move.to for en passant
    rook_from: int | None = None        # castling
    rook_to: int | None = None
    promoted_to: str | None = None      # FEN letter of the new piece


@dataclass
class Position:
    board: list[str | None] = field(default_factory=lambda: [None] * 64)
    white_to_move: bool = True
    castling: str = "KQkq"
    ep: int | None = None
    halfmove: int = 0
    fullmove: int = 1

    # ------------------------------------------------------------------ FEN
    @classmethod
    def from_fen(cls, fen: str = START_FEN) -> "Position":
        parts = fen.split()
        if len(parts) < 4:
            raise ValueError("FEN needs at least 4 fields")
        rows = parts[0].split("/")
        if len(rows) != 8:
            raise ValueError("FEN board must have 8 ranks")
        board: list[str | None] = [None] * 64
        for r, row in enumerate(rows):
            f = 0
            for ch in row:
                if ch.isdigit():
                    f += int(ch)
                elif ch in "pnbrqkPNBRQK":
                    if f > 7:
                        raise ValueError("FEN rank too long")
                    board[(7 - r) * 8 + f] = ch
                    f += 1
                else:
                    raise ValueError(f"bad FEN character {ch!r}")
            if f != 8:
                raise ValueError("FEN rank has wrong length")
        pos = cls(board=board, white_to_move=parts[1] == "w",
                  castling="" if parts[2] == "-" else parts[2],
                  ep=None if parts[3] == "-" else sq_index(parts[3]))
        if len(parts) >= 6:
            pos.halfmove, pos.fullmove = int(parts[4]), int(parts[5])
        return pos

    def fen(self) -> str:
        rows = []
        for r in range(7, -1, -1):
            row, empty = "", 0
            for f in range(8):
                p = self.board[r * 8 + f]
                if p is None:
                    empty += 1
                else:
                    row += (str(empty) if empty else "") + p
                    empty = 0
            rows.append(row + (str(empty) if empty else ""))
        return " ".join([
            "/".join(rows), "w" if self.white_to_move else "b", self.castling or "-",
            sq_name(self.ep) if self.ep is not None else "-", str(self.halfmove), str(self.fullmove),
        ])

    def copy(self) -> "Position":
        return Position(self.board[:], self.white_to_move, self.castling, self.ep,
                        self.halfmove, self.fullmove)

    # -------------------------------------------------------------- attacks
    def attacked(self, sq: int, by_white: bool) -> bool:
        f, r = sq % 8, sq // 8
        b = self.board
        pawn_r = r - 1 if by_white else r + 1
        for df in (-1, 1):
            if _on(f + df, pawn_r):
                p = b[pawn_r * 8 + f + df]
                if p == ("P" if by_white else "p"):
                    return True
        for df, dr in KNIGHT:
            if _on(f + df, r + dr):
                p = b[(r + dr) * 8 + f + df]
                if p == ("N" if by_white else "n"):
                    return True
        for df, dr in KING:
            if _on(f + df, r + dr):
                p = b[(r + dr) * 8 + f + df]
                if p == ("K" if by_white else "k"):
                    return True
        for dirs, kinds in ((ROOK_DIRS, "RQ"), (BISHOP_DIRS, "BQ")):
            if not by_white:
                kinds = kinds.lower()
            for df, dr in dirs:
                ff, rr = f + df, r + dr
                while _on(ff, rr):
                    p = b[rr * 8 + ff]
                    if p is not None:
                        if p in kinds:
                            return True
                        break
                    ff, rr = ff + df, rr + dr
        return False

    def king_square(self, white: bool) -> int:
        k = "K" if white else "k"
        return self.board.index(k)

    def in_check(self) -> bool:
        return self.attacked(self.king_square(self.white_to_move), not self.white_to_move)

    # ---------------------------------------------------------- generation
    def pseudo_moves(self) -> list[Move]:
        b, white = self.board, self.white_to_move
        moves: list[Move] = []
        for sq, p in enumerate(b):
            if p is None or is_white(p) != white:
                continue
            f, r = sq % 8, sq // 8
            kind = p.upper()
            if kind == "P":
                step = 1 if white else -1
                start, last = (1, 7) if white else (6, 0)
                r1 = r + step
                if _on(f, r1) and b[r1 * 8 + f] is None:
                    self._add_pawn(moves, sq, r1 * 8 + f, r1 == last)
                    r2 = r + 2 * step
                    if r == start and b[r2 * 8 + f] is None:
                        moves.append(Move(sq, r2 * 8 + f))
                for df in (-1, 1):
                    if _on(f + df, r1):
                        t = r1 * 8 + f + df
                        q = b[t]
                        if (q is not None and is_white(q) != white) or t == self.ep:
                            self._add_pawn(moves, sq, t, r1 == last)
            elif kind in "NK":
                for df, dr in (KNIGHT if kind == "N" else KING):
                    if _on(f + df, r + dr):
                        t = (r + dr) * 8 + f + df
                        q = b[t]
                        if q is None or is_white(q) != white:
                            moves.append(Move(sq, t))
                if kind == "K":
                    self._add_castles(moves, sq)
            else:
                dirs = {"B": BISHOP_DIRS, "R": ROOK_DIRS, "Q": BISHOP_DIRS + ROOK_DIRS}[kind]
                for df, dr in dirs:
                    ff, rr = f + df, r + dr
                    while _on(ff, rr):
                        t = rr * 8 + ff
                        q = b[t]
                        if q is None:
                            moves.append(Move(sq, t))
                        else:
                            if is_white(q) != white:
                                moves.append(Move(sq, t))
                            break
                        ff, rr = ff + df, rr + dr
        return moves

    @staticmethod
    def _add_pawn(moves: list[Move], frm: int, to: int, promo: bool) -> None:
        if promo:
            moves.extend(Move(frm, to, pc) for pc in "qrbn")
        else:
            moves.append(Move(frm, to))

    def _add_castles(self, moves: list[Move], ksq: int) -> None:
        white = self.white_to_move
        home = 4 if white else 60
        if ksq != home or self.attacked(home, not white):
            return
        b = self.board
        rights = self.castling
        k, q = ("K", "Q") if white else ("k", "q")
        rook = "R" if white else "r"
        if k in rights and b[home + 3] == rook and b[home + 1] is None and b[home + 2] is None:
            if not self.attacked(home + 1, not white) and not self.attacked(home + 2, not white):
                moves.append(Move(home, home + 2))
        if q in rights and b[home - 4] == rook and all(b[home - i] is None for i in (1, 2, 3)):
            if not self.attacked(home - 1, not white) and not self.attacked(home - 2, not white):
                moves.append(Move(home, home - 2))

    def legal_moves(self) -> list[Move]:
        out = []
        for m in self.pseudo_moves():
            nxt = self.copy()
            nxt._apply(m)
            if not nxt.attacked(nxt.king_square(self.white_to_move), nxt.white_to_move):
                out.append(m)
        return out

    # -------------------------------------------------------------- making
    def describe(self, m: Move) -> MoveInfo:
        """Physical consequences of a (legal) move, without playing it."""
        p = self.board[m.frm]
        if p is None:
            raise ValueError(f"no piece on {sq_name(m.frm)}")
        info = MoveInfo(move=m, piece=p)
        target = self.board[m.to]
        if target is not None:
            info.captured, info.captured_on = target, m.to
        elif p.upper() == "P" and m.to == self.ep and m.frm % 8 != m.to % 8:
            cap = m.to - 8 if is_white(p) else m.to + 8
            info.captured, info.captured_on = self.board[cap], cap
        if p.upper() == "K" and abs(m.to - m.frm) == 2:
            if m.to > m.frm:
                info.rook_from, info.rook_to = m.frm + 3, m.frm + 1
            else:
                info.rook_from, info.rook_to = m.frm - 4, m.frm - 1
        if m.promo:
            info.promoted_to = m.promo.upper() if is_white(p) else m.promo
        return info

    def _apply(self, m: Move) -> MoveInfo:
        info = self.describe(m)
        b = self.board
        if info.captured_on is not None:
            b[info.captured_on] = None
        b[m.frm] = None
        b[m.to] = info.promoted_to or info.piece
        if info.rook_from is not None:
            b[info.rook_to] = b[info.rook_from]
            b[info.rook_from] = None
        # castling rights
        for sq, right in ((4, "KQ"), (60, "kq"), (0, "Q"), (7, "K"), (56, "q"), (63, "k")):
            if m.frm == sq or m.to == sq:
                self.castling = "".join(c for c in self.castling if c not in right)
        # en passant square
        self.ep = None
        if info.piece.upper() == "P" and abs(m.to - m.frm) == 16:
            self.ep = (m.frm + m.to) // 2
        # clocks
        if info.piece.upper() == "P" or info.captured:
            self.halfmove = 0
        else:
            self.halfmove += 1
        if not self.white_to_move:
            self.fullmove += 1
        self.white_to_move = not self.white_to_move
        return info

    def push(self, m: Move | str) -> MoveInfo:
        """Play a legal move (Move or UCI string). Raises ValueError if illegal."""
        if isinstance(m, str):
            m = Move.from_uci(m)
        legal = self.legal_moves()
        if m not in legal:
            # accept e7e8 without a promotion letter as a queen promotion
            if m.promo is None and Move(m.frm, m.to, "q") in legal:
                m = Move(m.frm, m.to, "q")
            else:
                raise ValueError(f"illegal move {m.uci()} in {self.fen()}")
        return self._apply(m)

    # ------------------------------------------------------------- outcome
    def outcome(self) -> str | None:
        """'checkmate', 'stalemate', 'fifty-move', 'insufficient' or None."""
        if not self.legal_moves():
            return "checkmate" if self.in_check() else "stalemate"
        if self.halfmove >= 100:
            return "fifty-move"
        rest = sorted(p.upper() for p in self.board if p and p.upper() != "K")
        if rest in ([], ["B"], ["N"]):
            return "insufficient"
        return None

    def __str__(self) -> str:
        lines = []
        for r in range(7, -1, -1):
            lines.append(f"{r + 1} " + " ".join(self.board[r * 8 + f] or "." for f in range(8)))
        lines.append("  a b c d e f g h")
        return "\n".join(lines)


def perft(pos: Position, depth: int) -> int:
    if depth == 0:
        return 1
    n = 0
    for m in pos.legal_moves():
        nxt = pos.copy()
        nxt._apply(m)
        n += perft(nxt, depth - 1)
    return n
