#include "kinematics.h"

#include <math.h>

#include "config.h"

namespace {

const float kPi = 3.14159265358979f;
const float kRad2Deg = 180.0f / kPi;

bool side(float dx, float dy, float sign, float &angle) {
  const float r = sqrtf(dx * dx + dy * dy);
  if (r < 1e-6f) return false;
  const float c = (FB_L1 * FB_L1 + r * r - FB_L2 * FB_L2) / (2.0f * FB_L1 * r);
  if (c > 1.0f || c < -1.0f) return false;
  angle = atan2f(dy, dx) + sign * acosf(c);
  return true;
}

float angle_between(float ux, float uy, float vx, float vy) {
  const float cross = ux * vy - uy * vx;
  return atan2f(fabsf(cross), ux * vx + uy * vy) * kRad2Deg;
}

}  // namespace

namespace fb {

bool ik(Vec2 p, Angles &out) {
  return side(p.x + FB_D / 2.0f, p.y, +1.0f, out.left) &&
         side(p.x - FB_D / 2.0f, p.y, -1.0f, out.right);
}

bool fk(Angles a, Vec2 &out) {
  const float b1x = -FB_D / 2.0f + FB_L1 * cosf(a.left);
  const float b1y = FB_L1 * sinf(a.left);
  const float b2x = FB_D / 2.0f + FB_L1 * cosf(a.right);
  const float b2y = FB_L1 * sinf(a.right);
  const float vx = b2x - b1x, vy = b2y - b1y;
  const float dist = sqrtf(vx * vx + vy * vy);
  if (dist < 1e-6f || dist > 2.0f * FB_L2) return false;
  const float h = sqrtf(fmaxf(0.0f, FB_L2 * FB_L2 - 0.25f * dist * dist));
  out.x = 0.5f * (b1x + b2x) - h * vy / dist;
  out.y = 0.5f * (b1y + b2y) + h * vx / dist;
  return true;
}

float margin_deg(Vec2 p, float *trans_deg, float *elbow_deg) {
  Angles a;
  if (!ik(p, a)) return -1.0f;
  const float b1x = -FB_D / 2.0f + FB_L1 * cosf(a.left);
  const float b1y = FB_L1 * sinf(a.left);
  const float b2x = FB_D / 2.0f + FB_L1 * cosf(a.right);
  const float b2y = FB_L1 * sinf(a.right);
  const float trans = angle_between(b1x - p.x, b1y - p.y, b2x - p.x, b2y - p.y);
  const float e1 = angle_between(-FB_D / 2.0f - b1x, -b1y, p.x - b1x, p.y - b1y);
  const float e2 = angle_between(FB_D / 2.0f - b2x, -b2y, p.x - b2x, p.y - b2y);
  const float elbow = fminf(e1, e2);
  if (trans_deg) *trans_deg = trans;
  if (elbow_deg) *elbow_deg = elbow;
  return fminf(fminf(trans, 180.0f - trans), fminf(elbow, 180.0f - elbow));
}

bool is_safe(Vec2 p) {
  float trans, elbow;
  if (margin_deg(p, &trans, &elbow) < 0.0f) return false;
  return fminf(trans, 180.0f - trans) >= FB_MIN_TRANS_DEG &&
         fminf(elbow, 180.0f - elbow) >= FB_MIN_ELBOW_DEG;
}

long angle_to_steps(float rad, float home_deg) {
  const float deg = rad * kRad2Deg - home_deg;
  return lroundf(deg * FB_STEPS_PER_REV / 360.0f);
}

float steps_to_angle(long steps, float home_deg) {
  return ((float)steps * 360.0f / FB_STEPS_PER_REV + home_deg) / kRad2Deg;
}

}  // namespace fb
