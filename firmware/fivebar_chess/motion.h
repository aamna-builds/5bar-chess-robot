// Coordinated, straight-line moves of the effector (Arduino side).
#pragma once

#include "kinematics.h"

namespace motion {

enum class Result { Ok, Unsafe, Unreachable };

void begin();
void set_feed(float mm_s);
float feed();
void enable(bool on);

// Declare the current physical pose to be the home pose.
void zero();

// Current effector position (mm), from the step counters.
Vec2 position();

// Move in a straight line to target. The whole line is checked against the
// safety limits first; nothing moves if any point fails.
Result line_to(Vec2 target);

}  // namespace motion
