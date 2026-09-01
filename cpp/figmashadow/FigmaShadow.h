#pragma once

#include <string>

#include "Color.h"  // re-exported: figmashadow::parseColor
#include "Types.h"

namespace figmashadow {

// High-level entry point used by both platform glue layers.
//
// Parses `boxShadow`, rasterizes every layer into one premultiplied RGBA8888
// bitmap sized to (content + bleed) * scale device pixels, and memoizes the
// result: identical requests (e.g. every row of a list) reuse one bitmap.
//
// `bleed*` must be the same values the JS layer used to inflate the native
// view, so the bitmap lines up 1:1 with the view's pixels.
Bitmap render(float contentWidth, float contentHeight,
              float radiusTopLeft, float radiusTopRight,
              float radiusBottomRight, float radiusBottomLeft,
              const std::string& boxShadow,
              float bleedLeft, float bleedTop, float bleedRight, float bleedBottom,
              float scale, bool highQuality = false);

// Drops every memoized bitmap. Call on a memory warning.
void clearCache();

// Approximate number of bytes held by the cache.
size_t cacheSizeBytes();

}  // namespace figmashadow
