#include "Parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "Color.h"

namespace figmashadow {

namespace {

std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\n\r\f\v");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\n\r\f\v");
  return s.substr(a, b - a + 1);
}

std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

// Splits on the given delimiter, ignoring delimiters nested inside parentheses
// (so `rgba(0, 0, 0, .5)` survives a comma split).
std::vector<std::string> splitTopLevel(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;
  for (char c : s) {
    if (c == '(') depth++;
    else if (c == ')') depth = std::max(0, depth - 1);

    if (c == delim && depth == 0) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

// Whitespace tokenizer that keeps `fn( ... )` groups intact.
std::vector<std::string> tokenize(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;
  for (char c : s) {
    if (c == '(') depth++;
    else if (c == ')') depth = std::max(0, depth - 1);

    if (std::isspace(static_cast<unsigned char>(c)) && depth == 0) {
      if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

// Parses a CSS length. Accepts a bare number or one suffixed with `px`/`dp`/`pt`
// (all treated as logical px). Rejects other units so they don't get mistaken
// for offsets.
bool parseLength(const std::string& tok, float& out) {
  if (tok.empty()) return false;
  char* end = nullptr;
  double v = std::strtod(tok.c_str(), &end);
  if (end == tok.c_str()) return false;
  std::string unit = toLower(trim(std::string(end)));
  if (unit.empty() || unit == "px" || unit == "dp" || unit == "dip" || unit == "pt") {
    out = static_cast<float>(v);
    return true;
  }
  return false;
}

ShadowLayer parseLayer(const std::string& raw, bool& ok) {
  ShadowLayer layer;
  ok = false;

  std::vector<float> lengths;
  bool hasColor = false;

  for (const auto& tok : tokenize(raw)) {
    std::string lower = toLower(tok);
    if (lower == "inset") {
      layer.inset = true;
      continue;
    }
    float len;
    Color color;
    // A bare number is always a length; try length before color so tokens like
    // `0` aren't swallowed by a lenient color parser.
    if (parseLength(tok, len)) {
      lengths.push_back(len);
    } else if (!hasColor && parseColor(tok, color)) {
      layer.color = color;
      hasColor = true;
    } else {
      // Unknown token: ignore it rather than dropping the whole layer.
    }
  }

  if (lengths.size() < 2) return layer;

  layer.offsetX = lengths[0];
  layer.offsetY = lengths[1];
  if (lengths.size() >= 3) layer.blur = std::max(0.0f, lengths[2]);
  if (lengths.size() >= 4) layer.spread = lengths[3];
  if (!hasColor) layer.color = {0.0f, 0.0f, 0.0f, 1.0f};  // CSS default is currentColor

  ok = true;
  return layer;
}

}  // namespace

std::vector<ShadowLayer> parseBoxShadow(const std::string& input) {
  std::string s = trim(input);

  // Strip an optional `box-shadow:` / `boxShadow:` prefix.
  size_t colon = s.find(':');
  if (colon != std::string::npos) {
    std::string head = toLower(trim(s.substr(0, colon)));
    if (head == "box-shadow" || head == "boxshadow") {
      s = trim(s.substr(colon + 1));
    }
  }
  if (!s.empty() && s.back() == ';') s.pop_back();
  s = trim(s);

  std::vector<ShadowLayer> layers;
  if (s.empty() || toLower(s) == "none") return layers;

  for (const auto& part : splitTopLevel(s, ',')) {
    std::string layerStr = trim(part);
    if (layerStr.empty()) continue;
    bool ok = false;
    ShadowLayer layer = parseLayer(layerStr, ok);
    if (ok) layers.push_back(layer);
  }
  return layers;
}

}  // namespace figmashadow
