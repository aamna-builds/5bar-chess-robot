// 5-bar chess robot firmware (Arduino Uno + 2x TB6600 + 2x geared NEMA 17).
//
// Receives one command per line over USB serial at 115200 baud and answers
// one line ("ok", "err <reason>" or a status line). The host program in
// host/ plans chess moves and sends straight-line moves; this sketch does the
// inverse kinematics, refuses anything outside the safe workspace, and
// generates coordinated step pulses with a trapezoidal speed profile.
//
// Commands:  G x y | F mm/s | M1 | M0 | Z | H | ? | E1 | E0

#include <Servo.h>

#include "config.h"
#include "kinematics.h"
#include "motion.h"
#include "protocol.h"

namespace {

Servo g_servo;
bool g_magnet = false;
bool g_zeroed = false;
char g_line[48];
uint8_t g_len = 0;
bool g_overflow = false;

void magnet(bool on) {
  g_servo.write(on ? FB_SERVO_ENGAGED : FB_SERVO_RELEASED);
  if (on != g_magnet) delay(FB_MAGNET_SETTLE_MS);
  g_magnet = on;
}

void reply_status() {
  const Vec2 p = motion::position();
  Serial.print(F("pos "));
  Serial.print(p.x, 2);
  Serial.print(' ');
  Serial.print(p.y, 2);
  Serial.print(F(" mag "));
  Serial.print(g_magnet ? 1 : 0);
  Serial.print(F(" zeroed "));
  Serial.println(g_zeroed ? 1 : 0);
}

void reply_move(motion::Result r) {
  switch (r) {
    case motion::Result::Ok: Serial.println(F("ok")); break;
    case motion::Result::Unsafe: Serial.println(F("err unsafe")); break;
    case motion::Result::Unreachable: Serial.println(F("err unreachable")); break;
  }
}

Vec2 home_point() {
  Angles a = {FB_HOME_LEFT_DEG * (float)M_PI / 180.0f, FB_HOME_RIGHT_DEG * (float)M_PI / 180.0f};
  Vec2 p;
  fb::fk(a, p);
  return p;
}

void handle(const char *line) {
  const Command c = parse_line(line);
  switch (c.type) {
    case Cmd::None: return;
    case Cmd::Invalid: Serial.println(F("err syntax")); return;
    case Cmd::Status: reply_status(); return;
    case Cmd::Zero:
      motion::zero();
      g_zeroed = true;
      Serial.println(F("ok"));
      return;
    case Cmd::Feed:
      motion::set_feed(c.a);
      Serial.println(F("ok"));
      return;
    case Cmd::Magnet:
      magnet(c.a > 0.5f);
      Serial.println(F("ok"));
      return;
    case Cmd::Enable:
      motion::enable(c.a > 0.5f);
      Serial.println(F("ok"));
      return;
    case Cmd::Move:
    case Cmd::Home:
      if (!g_zeroed) {             // never move before the arms are calibrated
        Serial.println(F("err not zeroed (put arms in home pose, send Z)"));
        return;
      }
      if (c.type == Cmd::Home) {
        reply_move(motion::line_to(home_point()));
      } else {
        reply_move(motion::line_to({c.a, c.b}));
      }
      return;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  g_servo.attach(FB_SERVO_PIN);
  g_servo.write(FB_SERVO_RELEASED);
  motion::begin();
  Serial.println(F("fivebar-chess ready; put arms in home pose and send Z"));
}

void loop() {
  while (Serial.available() > 0) {
    const char ch = (char)Serial.read();
    if (ch == '\n') {
      g_line[g_len] = '\0';
      if (g_overflow) Serial.println(F("err line too long"));
      else handle(g_line);
      g_len = 0;
      g_overflow = false;
    } else if (g_len < sizeof(g_line) - 1) {
      g_line[g_len++] = ch;
    } else {
      g_overflow = true;
    }
  }
}
