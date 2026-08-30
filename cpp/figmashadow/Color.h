#pragma once

#include <string>

#include "Types.h"

namespace figmashadow {

// Parses a CSS color token: #rgb, #rgba, #rrggbb, #rrggbbaa, rgb()/rgba(),
// hsl()/hsla(), `transparent`, `currentColor` (treated as opaque black) and the
// common CSS named colors. Returns false if `token` is not a color.
bool parseColor(const std::string& token, Color& out);

}  // namespace figmashadow
