"""Host-side software for the 5-bar chess robot.

Modules
-------
config      load config/robot.json
kinematics  inverse/forward kinematics and safety checks (mirrors matlab/)
chess       a small, self-contained chess rules engine (no third-party code)
board       square <-> millimetre mapping and graveyard slots
planner     turns a chess move into collision-free magnet paths
robot       serial driver for the firmware and an offline simulator
engine      optional UCI engine (e.g. Stockfish) running as a separate program
"""

__version__ = "2.0.0"
