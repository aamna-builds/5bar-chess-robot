// Line protocol between the host and the firmware (see README / host/robot.py).
// Plain C++ so it can be unit-tested on a PC.
#pragma once

enum class Cmd {
  None,      // empty line
  Move,      // G x y
  Feed,      // F mm/s
  Magnet,    // M1 / M0
  Zero,      // Z   (current pose = home pose)
  Home,      // H   (travel back to the home pose)
  Status,    // ?
  Enable,    // E1 / E0
  Invalid
};

struct Command {
  Cmd type = Cmd::None;
  float a = 0.0f;   // x, feed, or flag
  float b = 0.0f;   // y
};

// Parse one line (without the newline). Never reads past the terminating NUL.
Command parse_line(const char *line);
