// 5-bar linkage kinematics for the firmware. Plain C++ (no Arduino calls) so
// it is unit-tested on a PC as well: see firmware/test/.
// Frame and conventions are identical to matlab/fb_ik.m and host/kinematics.py.
#pragma once

struct Vec2 {
  float x;
  float y;
};

struct Angles {
  float left;   // rad, CCW from +x
  float right;
};

namespace fb {

// Inverse kinematics, elbows-out working mode. Returns false if out of reach.
bool ik(Vec2 p, Angles &out);

// Forward kinematics, effector on the far side of the elbows.
bool fk(Angles a, Vec2 &out);

// Distance (deg) of pose p from both singularity types; negative if unreachable.
float margin_deg(Vec2 p, float *trans_deg = nullptr, float *elbow_deg = nullptr);

// True when p is reachable and inside the configured safety limits.
bool is_safe(Vec2 p);

// Motor angle (rad) -> step count relative to the home pose, and back.
long angle_to_steps(float rad, float home_deg);
float steps_to_angle(long steps, float home_deg);

}  // namespace fb
