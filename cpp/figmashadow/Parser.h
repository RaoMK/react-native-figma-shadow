#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace figmashadow {

// Parses a CSS `box-shadow` value into layers, in source order (the first layer
// paints on top). Tolerates a leading `box-shadow:` and a trailing `;`, the
// `inset` keyword in any position, 2-4 lengths, and a color token anywhere in
// the declaration. Unparseable layers are skipped.
std::vector<ShadowLayer> parseBoxShadow(const std::string& input);

}  // namespace figmashadow
