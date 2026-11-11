import pytest

from fivebar_chess.chess import START_FEN, Move, Position, perft

# Standard perft reference counts (chessprogramming.org "Perft Results")
PERFT = [
    (START_FEN, [20, 400, 8902]),
    ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", [48, 2039, 97862]),
    ("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", [14, 191, 2812, 43238]),
    ("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", [6, 264, 9467]),
    ("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", [44, 1486, 62379]),
    ("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", [46, 2079]),
]


@pytest.mark.parametrize("fen,counts", PERFT)
def test_perft(fen, counts):
    pos = Position.from_fen(fen)
    for depth, n in enumerate(counts, start=1):
        assert perft(pos, depth) == n, f"depth {depth}"


def test_fen_round_trip():
    for fen, _ in PERFT:
        assert Position.from_fen(fen).fen() == fen


def test_fools_mate():
    pos = Position.from_fen()
    for m in "f2f3 e7e5 g2g4 d8h4".split():
        pos.push(m)
    assert pos.outcome() == "checkmate"


def test_stalemate():
    pos = Position.from_fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1")
    assert pos.outcome() == "stalemate"


def test_illegal_move_rejected():
    pos = Position.from_fen()
    with pytest.raises(ValueError):
        pos.push("e2e5")
    with pytest.raises(ValueError):
        pos.push("e1e2")      # own pawn there


def test_special_move_descriptions():
    pos = Position.from_fen("r3k2r/8/8/3pP3/8/8/1p6/R3K2R w KQkq d6 0 1")
    ep = pos.describe(Move.from_uci("e5d6"))
    assert ep.captured == "p" and ep.captured_on == Move.from_uci("d5d5").frm
    oo = pos.describe(Move.from_uci("e1g1"))
    assert (oo.rook_from, oo.rook_to) == (7, 5)
    ooo = pos.describe(Move.from_uci("e1c1"))
    assert (ooo.rook_from, ooo.rook_to) == (0, 3)
    pos.push("a1a2")
    promo = pos.push("b2b1n")
    assert promo.promoted_to == "n"


def test_castling_rights_lost_when_rook_captured():
    pos = Position.from_fen("r3k2r/8/8/8/8/8/6b1/R3K2R b KQkq - 0 1")
    pos.push("g2h1")
    assert "K" not in pos.castling and "Q" in pos.castling


def test_bare_promotion_defaults_to_queen():
    pos = Position.from_fen("8/P7/8/8/8/8/8/k6K w - - 0 1")
    info = pos.push("a7a8")
    assert info.promoted_to == "Q"
