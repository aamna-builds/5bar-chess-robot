# Firmware (Arduino Uno)

```
fivebar_chess/
  fivebar_chess.ino   serial line handling, magnet servo, command dispatch
  motion.cpp/.h       straight-line moves: safety pre-check, IK every 1 mm, trapezoidal speed, Bresenham step interleaving
  kinematics.cpp/.h   IK / FK / safety margins (no Arduino calls, unit-tested on a PC)
  protocol.cpp/.h     line parser (no Arduino calls, unit-tested on a PC)
  config.h            generated from config/robot.json, do not edit
test/test_native.cpp  PC tests, including a check against matlab/output/square_table.csv
```

The only library used is `Servo`, which ships with the Arduino IDE. All stepping code is written from scratch.

## Build and flash

Arduino IDE: open `fivebar_chess/fivebar_chess.ino`, select **Arduino Uno**, then Upload.

arduino-cli:

```bash
arduino-cli core install arduino:avr && arduino-cli lib install Servo
tools/build_firmware.sh                        # compile
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:uno firmware/fivebar_chess
```

PC tests (g++):

```bash
tools/test_firmware.sh
```

To run the as-built v1 geometry instead of v2:

```bash
python3 tools/gen_firmware_config.py --config config/robot_v1.json
```

The build uses about 45 % of the Uno's flash and 16 % of its RAM.

## Commands (115200 baud, one per line)

| Command | Meaning | Reply |
|---|---|---|
| `Z` | the arms are in the home pose now (calibrate) | `ok` |
| `G x y` | straight line to (x, y) mm | `ok` when done, `err unsafe` or `err unreachable` |
| `F v` | travel speed in mm/s (capped at `max_speed_mm_s`) | `ok` |
| `M1` / `M0` | magnet towards / away from the board | `ok` |
| `H` | return to the home pose | `ok` |
| `E1` / `E0` | enable / disable the drivers (if `pins.enable` is wired) | `ok` |
| `?` | status | `pos x y mag 0/1 zeroed 0/1` |

Safety rules:

- **Calibration first:** moves are refused until `Z` has been sent.
- **Whole-line check:** the entire line is checked before the first step. If any point is closer than the configured margin to a singularity, the robot does not move at all.
