#include "Color.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace figmashadow {

namespace {

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\n\r\f\v");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\n\r\f\v");
  return s.substr(a, b - a + 1);
}

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

bool hexNibble(char c, int& out) {
  if (c >= '0' && c <= '9') { out = c - '0'; return true; }
  c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (c >= 'a' && c <= 'f') { out = c - 'a' + 10; return true; }
  return false;
}

bool parseHex(const std::string& tok, Color& out) {
  if (tok.empty() || tok[0] != '#') return false;
  std::string h = tok.substr(1);
  const size_t n = h.size();
  if (n != 3 && n != 4 && n != 6 && n != 8) return false;

  std::vector<int> nibbles;
  nibbles.reserve(n);
  for (char c : h) {
    int v;
    if (!hexNibble(c, v)) return false;
    nibbles.push_back(v);
  }

  int r, g, b, a = 255;
  if (n == 3 || n == 4) {
    r = nibbles[0] * 17;
    g = nibbles[1] * 17;
    b = nibbles[2] * 17;
    if (n == 4) a = nibbles[3] * 17;
  } else {
    r = nibbles[0] * 16 + nibbles[1];
    g = nibbles[2] * 16 + nibbles[3];
    b = nibbles[4] * 16 + nibbles[5];
    if (n == 8) a = nibbles[6] * 16 + nibbles[7];
  }
  out.r = r / 255.0f;
  out.g = g / 255.0f;
  out.b = b / 255.0f;
  out.a = a / 255.0f;
  return true;
}

// Splits the inside of `rgb( ... )` on commas and/or whitespace (and an
// optional `/` alpha separator), tolerating the modern and legacy syntaxes.
std::vector<std::string> splitArgs(const std::string& inner) {
  std::vector<std::string> parts;
  std::string cur;
  for (char c : inner) {
    if (c == ',' || c == '/' || std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) parts.push_back(cur);
  return parts;
}

// Parses a channel that is either `0..255` or a percentage `0..100%`.
bool parseChannel(const std::string& s, float& out) {
  if (s.empty()) return false;
  char* end = nullptr;
  double v = std::strtod(s.c_str(), &end);
  if (end == s.c_str()) return false;
  if (*end == '%') {
    out = clamp01(static_cast<float>(v / 100.0));
  } else {
    out = clamp01(static_cast<float>(v / 255.0));
  }
  return true;
}

bool parseAlpha(const std::string& s, float& out) {
  if (s.empty()) return false;
  char* end = nullptr;
  double v = std::strtod(s.c_str(), &end);
  if (end == s.c_str()) return false;
  if (*end == '%') v /= 100.0;
  out = clamp01(static_cast<float>(v));
  return true;
}

void hslToRgb(float h, float s, float l, Color& out) {
  h = std::fmod(std::fmod(h, 360.0f) + 360.0f, 360.0f) / 360.0f;
  auto hue = [](float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
  };
  if (s <= 0.0f) {
    out.r = out.g = out.b = l;
    return;
  }
  float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
  float p = 2.0f * l - q;
  out.r = hue(p, q, h + 1.0f / 3.0f);
  out.g = hue(p, q, h);
  out.b = hue(p, q, h - 1.0f / 3.0f);
}

bool parseFunctional(const std::string& tok, Color& out) {
  size_t open = tok.find('(');
  if (open == std::string::npos || tok.back() != ')') return false;
  std::string fn = toLower(tok.substr(0, open));
  std::string inner = tok.substr(open + 1, tok.size() - open - 2);
  auto args = splitArgs(inner);

  if (fn == "rgb" || fn == "rgba") {
    if (args.size() < 3) return false;
    float r, g, b, a = 1.0f;
    if (!parseChannel(args[0], r) || !parseChannel(args[1], g) ||
        !parseChannel(args[2], b))
      return false;
    if (args.size() >= 4 && !parseAlpha(args[3], a)) return false;
    out = {r, g, b, a};
    return true;
  }
  if (fn == "hsl" || fn == "hsla") {
    if (args.size() < 3) return false;
    char* e = nullptr;
    float h = static_cast<float>(std::strtod(args[0].c_str(), &e));
    if (e == args[0].c_str()) return false;
    auto pct = [](const std::string& s, float& v) {
      char* end = nullptr;
      double d = std::strtod(s.c_str(), &end);
      if (end == s.c_str()) return false;
      v = static_cast<float>(d / 100.0);
      return true;
    };
    float s, l, a = 1.0f;
    if (!pct(args[1], s) || !pct(args[2], l)) return false;
    s = clamp01(s);
    l = clamp01(l);
    if (args.size() >= 4 && !parseAlpha(args[3], a)) return false;
    hslToRgb(h, s, l, out);
    out.a = a;
    return true;
  }
  return false;
}

struct NamedColor {
  const char* name;
  uint32_t rgb;  // 0xRRGGBB
};

// The most common CSS named colors. rgb()/rgba()/hex cover the rest and are what
// Figma and design tools actually emit.
constexpr std::array<NamedColor, 34> kNamed = {{
    {"black", 0x000000},   {"white", 0xffffff},   {"red", 0xff0000},
    {"green", 0x008000},   {"blue", 0x0000ff},    {"yellow", 0xffff00},
    {"cyan", 0x00ffff},    {"magenta", 0xff00ff}, {"gray", 0x808080},
    {"grey", 0x808080},    {"silver", 0xc0c0c0},  {"maroon", 0x800000},
    {"olive", 0x808000},   {"lime", 0x00ff00},    {"aqua", 0x00ffff},
    {"teal", 0x008080},    {"navy", 0x000080},    {"fuchsia", 0xff00ff},
    {"purple", 0x800080},  {"orange", 0xffa500},  {"pink", 0xffc0cb},
    {"brown", 0xa52a2a},   {"gold", 0xffd700},    {"indigo", 0x4b0082},
    {"violet", 0xee82ee},  {"crimson", 0xdc143c}, {"coral", 0xff7f50},
    {"salmon", 0xfa8072},  {"khaki", 0xf0e68c},   {"lavender", 0xe6e6fa},
    {"plum", 0xdda0dd},    {"orchid", 0xda70d6},  {"tan", 0xd2b48c},
    {"turquoise", 0x40e0d0},
}};

}  // namespace

bool parseColor(const std::string& raw, Color& out) {
  std::string tok = trim(raw);
  if (tok.empty()) return false;

  if (tok[0] == '#') return parseHex(tok, out);
  if (tok.find('(') != std::string::npos) return parseFunctional(tok, out);

  std::string lower = toLower(tok);
  if (lower == "transparent") {
    out = {0.0f, 0.0f, 0.0f, 0.0f};
    return true;
  }
  if (lower == "currentcolor") {
    out = {0.0f, 0.0f, 0.0f, 1.0f};
    return true;
  }
  for (const auto& nc : kNamed) {
    if (lower == nc.name) {
      out.r = ((nc.rgb >> 16) & 0xff) / 255.0f;
      out.g = ((nc.rgb >> 8) & 0xff) / 255.0f;
      out.b = (nc.rgb & 0xff) / 255.0f;
      out.a = 1.0f;
      return true;
    }
  }
  return false;
}

}  // namespace figmashadow
