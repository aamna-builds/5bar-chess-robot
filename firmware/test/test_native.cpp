// PC-side unit tests for the firmware's pure C++ parts (kinematics + protocol).
// Built and run by tools/test_firmware.sh and in CI. Uses 32-bit float maths
// exactly like the AVR, and checks it against the MATLAB lookup table.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../fivebar_chess/config.h"
#include "../fivebar_chess/kinematics.h"
#include "../fivebar_chess/protocol.h"

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, ...)                                  \
  do {                                                    \
    if (cond) { ++g_pass; }                               \
    else { ++g_fail; std::printf("FAIL %s:%d ", __FILE__, __LINE__); \
           std::printf(__VA_ARGS__); std::printf("\n"); } \
  } while (0)

static void test_protocol() {
  Command c = parse_line("G 12.5 -3");
  CHECK(c.type == Cmd::Move && c.a == 12.5f && c.b == -3.0f, "G parse");
  CHECK(parse_line("g1 2").type == Cmd::Move, "lower case + no space");
  CHECK(parse_line("G 1").type == Cmd::Invalid, "G needs two numbers");
  CHECK(parse_line("G 1 2 3").type == Cmd::Invalid, "trailing garbage");
  CHECK(parse_line("G x y").type == Cmd::Invalid, "non-numbers");
  CHECK(parse_line("M1").type == Cmd::Magnet && parse_line("M1").a == 1.0f, "M1");
  CHECK(parse_line(" m0 \r").type == Cmd::Magnet && parse_line("M0").a == 0.0f, "M0");
  CHECK(parse_line("M2").type == Cmd::Invalid, "M2 invalid");
  CHECK(parse_line("F 80").type == Cmd::Feed, "F");
  CHECK(parse_line("F -5").type == Cmd::Invalid, "negative feed");
  CHECK(parse_line("Z").type == Cmd::Zero, "Z");
  CHECK(parse_line("H").type == Cmd::Home, "H");
  CHECK(parse_line("?").type == Cmd::Status, "?");
  CHECK(parse_line("E0").type == Cmd::Enable, "E0");
  CHECK(parse_line("").type == Cmd::None, "empty");
  CHECK(parse_line("X").type == Cmd::Invalid, "unknown");
}

static void test_round_trip() {
  int n = 0;
  float worst = 0;
  for (float x = -200; x <= 200; x += 10)
    for (float y = 130; y <= 510; y += 10) {
      Angles a;
      Vec2 p = {x, y}, q;
      if (!fb::ik(p, a)) continue;
      CHECK(fb::fk(a, q), "fk at %g %g", x, y);
      worst = std::fmax(worst, std::hypot(q.x - p.x, q.y - p.y));
      ++n;
    }
  CHECK(n > 1000, "round trip sampled %d points", n);
  CHECK(worst < 0.01f, "float round-trip error %g mm", worst);
}

static void test_against_matlab(const char *csv) {
  std::ifstream in(csv);
  if (!in) { std::printf("skip: %s not found (run matlab/run_all.m)\n", csv); return; }
  std::string line;
  std::getline(in, line);  // header
  int rows = 0;
  while (std::getline(in, line)) {
    std::stringstream ss(line);
    std::string name, f[8];
    std::getline(ss, name, ',');
    for (auto &s : f) std::getline(ss, s, ',');
    Vec2 p = {std::stof(f[0]), std::stof(f[1])};
    Angles a;
    CHECK(fb::ik(p, a), "%s reachable", name.c_str());
    const long s0 = fb::angle_to_steps(a.left, FB_HOME_LEFT_DEG);
    const long s1 = fb::angle_to_steps(a.right, FB_HOME_RIGHT_DEG);
    CHECK(std::labs(s0 - std::stol(f[4])) <= 1 && std::labs(s1 - std::stol(f[5])) <= 1,
          "%s steps %ld %ld vs MATLAB %s %s", name.c_str(), s0, s1, f[4].c_str(), f[5].c_str());
    CHECK(fb::is_safe(p), "%s safe", name.c_str());
    ++rows;
  }
  CHECK(rows >= 64 + 30, "rows %d", rows);
}

static void test_limits() {
  CHECK(!fb::is_safe({0, FB_L1 + FB_L2 + 10}), "beyond reach rejected");
  CHECK(!fb::is_safe({0, 5}), "between the motors rejected");
  Vec2 home;
  Angles h = {FB_HOME_LEFT_DEG * 3.14159265f / 180, FB_HOME_RIGHT_DEG * 3.14159265f / 180};
  CHECK(fb::fk(h, home) && fb::is_safe(home), "home pose is safe");
  CHECK(fb::angle_to_steps(fb::steps_to_angle(1234, 10.0f), 10.0f) == 1234, "steps round trip");
}

int main(int argc, char **argv) {
  test_protocol();
  test_round_trip();
  test_limits();
  test_against_matlab(argc > 1 ? argv[1] : "matlab/output/square_table.csv");
  std::printf("firmware native tests: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail ? 1 : 0;
}
