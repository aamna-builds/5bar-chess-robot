#include "protocol.h"

#include <ctype.h>
#include <stdlib.h>

namespace {

const char *skip_ws(const char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\r') ++s;
  return s;
}

// Parse a float; returns false if no number or trailing garbage.
bool number(const char *&s, float &out) {
  s = skip_ws(s);
  char *end = nullptr;
  const double v = strtod(s, &end);
  if (end == s) return false;
  out = (float)v;
  s = end;
  return true;
}

bool at_end(const char *s) { return *skip_ws(s) == '\0'; }

}  // namespace

Command parse_line(const char *line) {
  Command c;
  const char *s = skip_ws(line);
  if (*s == '\0') return c;
  const char op = (char)toupper((unsigned char)*s++);
  switch (op) {
    case 'G':
      c.type = (number(s, c.a) && number(s, c.b) && at_end(s)) ? Cmd::Move : Cmd::Invalid;
      break;
    case 'F':
      c.type = (number(s, c.a) && at_end(s) && c.a > 0.0f) ? Cmd::Feed : Cmd::Invalid;
      break;
    case 'M':
    case 'E': {
      s = skip_ws(s);
      if ((*s == '0' || *s == '1') && at_end(s + 1)) {
        c.type = op == 'M' ? Cmd::Magnet : Cmd::Enable;
        c.a = (float)(*s - '0');
      } else {
        c.type = Cmd::Invalid;
      }
      break;
    }
    case 'Z': c.type = at_end(s) ? Cmd::Zero : Cmd::Invalid; break;
    case 'H': c.type = at_end(s) ? Cmd::Home : Cmd::Invalid; break;
    case '?': c.type = at_end(s) ? Cmd::Status : Cmd::Invalid; break;
    default: c.type = Cmd::Invalid;
  }
  return c;
}
