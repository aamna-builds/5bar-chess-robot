# Host software (`fivebar_chess`)

The host is pure Python 3.9 or later with no required dependencies:

- `pyserial` is needed only to drive the real robot.
- `matplotlib` and `pillow` are needed only for the animations.

```bash
cd host
pip install -e ".[robot,sim]"         # or ".[dev]" to run the tests

python -m fivebar_chess check                                   # margin of every square
python -m fivebar_chess replay --sim examples/italian_game.txt --gif game.gif
python -m fivebar_chess play --sim                              # type moves, e.g. e2e4
python -m fivebar_chess play --port /dev/ttyUSB0 --engine /usr/games/stockfish --colour white
python -m fivebar_chess goto d4 --port COM3                     # calibration
python -m fivebar_chess --config ../config/robot_v1.json check  # the as-built geometry
```

## How a move becomes motion

1. **Rules:** `chess.py` checks that the move is legal and reports its side effects: a capture, an en-passant victim, the rook of a castle, or a promotion.
2. **Planning:** `planner.py` turns those into *transfers*, each one piece sliding from A to B:
   - **Captured piece:** goes first, to the next free graveyard slot of its colour.
   - **Moving piece:** goes straight if the line keeps at least `piece diameter + 2 mm` from every other piece. Otherwise it follows the lanes between squares, which always keep half a square of clearance.
   - **Castling rook:** moved after the king.
   - **Promotion:** the pawn goes to the graveyard, and a captured piece of the promoted type is brought back if one is available. If not, the program asks you to place it by hand.
3. **Safety:** every path is sampled every 2 mm through the inverse kinematics, and rejected if any sample is outside the singularity limits.
4. **Execution:** `robot.py` sends `M0`, a travel move to the start, `M1`, the carry waypoints, then `M0`. The firmware checks every line again before it moves.

## Tests

```bash
pytest -q
```

The tests cover:

- standard perft counts on six positions, which proves the move generator is exact
- FEN round trips, mate and stalemate
- every special move
- IK/FK round trips
- agreement with the MATLAB table for all 64 squares
- planning through two example games and six random 120-ply games, with every path re-checked for collisions and safety
