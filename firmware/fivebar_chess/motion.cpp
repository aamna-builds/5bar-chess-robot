#include "motion.h"

#include <Arduino.h>
#include <math.h>

#include "config.h"

namespace {

long g_steps[2] = {0, 0};          // relative to the home pose
float g_feed = FB_VMAX_MM_S;
const float kMinSpeed = 4.0f;      // mm/s, so very short segments still finish

const uint8_t kStep[2] = {FB_PIN_LEFT_STEP, FB_PIN_RIGHT_STEP};
const uint8_t kDir[2] = {FB_PIN_LEFT_DIR, FB_PIN_RIGHT_DIR};
const bool kInvert[2] = {FB_INVERT_LEFT, FB_INVERT_RIGHT};

inline void pulse(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(4);            // TB6600 needs >= 2.5 us
  digitalWrite(pin, LOW);
}

void set_dir(int m, long delta) {
  const bool forward = (delta >= 0) != kInvert[m];
  digitalWrite(kDir[m], forward ? HIGH : LOW);
}

// Run both motors by (d0, d1) steps in `us` microseconds, Bresenham-interleaved.
void run_segment(long d0, long d1, unsigned long us) {
  set_dir(0, d0);
  set_dir(1, d1);
  delayMicroseconds(6);            // direction setup time
  const long n0 = labs(d0), n1 = labs(d1);
  const long n = n0 > n1 ? n0 : n1;
  if (n == 0) return;
  const unsigned long dt = us / (unsigned long)n;
  long e0 = n / 2, e1 = n / 2;
  unsigned long t = micros();
  for (long i = 0; i < n; ++i) {
    e0 -= n0;
    if (e0 < 0) { e0 += n; pulse(kStep[0]); }
    e1 -= n1;
    if (e1 < 0) { e1 += n; pulse(kStep[1]); }
    t += dt;
    while ((long)(micros() - t) < 0) {}
  }
  g_steps[0] += d0;
  g_steps[1] += d1;
}

float profile_speed(float s, float length, float vmax) {
  // trapezoid: accelerate from rest, cruise, decelerate to rest
  const float a = FB_ACCEL_MM_S2;
  float v = sqrtf(2.0f * a * fmaxf(s, 0.0f));
  v = fminf(v, sqrtf(2.0f * a * fmaxf(length - s, 0.0f)));
  v = fminf(v, vmax);
  return fmaxf(v, kMinSpeed);
}

}  // namespace

namespace motion {

void begin() {
  for (int m = 0; m < 2; ++m) {
    pinMode(kStep[m], OUTPUT);
    pinMode(kDir[m], OUTPUT);
    digitalWrite(kStep[m], LOW);
  }
#if FB_PIN_ENABLE >= 0
  pinMode(FB_PIN_ENABLE, OUTPUT);
#endif
  enable(true);
}

void enable(bool on) {
#if FB_PIN_ENABLE >= 0
  const bool high = FB_ENABLE_OFF_HIGH ? !on : on;
  digitalWrite(FB_PIN_ENABLE, high ? HIGH : LOW);
#else
  (void)on;
#endif
}

void set_feed(float mm_s) { g_feed = fminf(fmaxf(mm_s, kMinSpeed), FB_VMAX_MM_S); }
float feed() { return g_feed; }

void zero() { g_steps[0] = g_steps[1] = 0; }

Vec2 position() {
  Angles a = {fb::steps_to_angle(g_steps[0], FB_HOME_LEFT_DEG),
              fb::steps_to_angle(g_steps[1], FB_HOME_RIGHT_DEG)};
  Vec2 p = {NAN, NAN};
  fb::fk(a, p);
  return p;
}

Result line_to(Vec2 target) {
  const Vec2 start = position();
  const float dx = target.x - start.x, dy = target.y - start.y;
  const float length = sqrtf(dx * dx + dy * dy);
  const int n = (int)ceilf(length / FB_SEGMENT_MM);

  // 1) check the whole line before moving anything. The safety margins vary
  //    smoothly, so sampling every kCheckMm (plus both ends) is enough and
  //    keeps the pause before a long move short on the 16 MHz AVR.
  const float kCheckMm = 5.0f;
  Angles a;
  if (!fb::ik(target, a)) return Result::Unreachable;
  const int checks = (int)ceilf(length / kCheckMm);
  for (int k = 0; k <= checks; ++k) {
    const float t = checks ? (float)k / checks : 1.0f;
    const Vec2 p = {start.x + t * dx, start.y + t * dy};
    if (!fb::is_safe(p)) return Result::Unsafe;
  }
  if (n == 0) return Result::Ok;

  // 2) execute segment by segment, re-solving the IK at every segment end
  const float seg = length / n;
  for (int k = 1; k <= n; ++k) {
    const float t = (float)k / n;
    const Vec2 p = {start.x + t * dx, start.y + t * dy};
    fb::ik(p, a);
    const long s0 = fb::angle_to_steps(a.left, FB_HOME_LEFT_DEG);
    const long s1 = fb::angle_to_steps(a.right, FB_HOME_RIGHT_DEG);
    const float v = profile_speed((k - 0.5f) * seg, length, g_feed);
    const unsigned long us = (unsigned long)(1e6f * seg / v);
    run_segment(s0 - g_steps[0], s1 - g_steps[1], us);
  }
  return Result::Ok;
}

}  // namespace motion
