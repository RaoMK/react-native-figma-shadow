#pragma once

#include <cstdint>
#include <vector>

namespace figmashadow {

// Straight (non-premultiplied) RGBA, components in [0, 1].
struct Color {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
};

// A single CSS `box-shadow` layer.
struct ShadowLayer {
  float offsetX = 0.0f;  // logical px
  float offsetY = 0.0f;  // logical px
  float blur = 0.0f;     // CSS blur radius, logical px, always >= 0
  float spread = 0.0f;   // logical px, may be negative
  Color color;
  bool inset = false;
};

// Per-corner border radii, logical px.
struct CornerRadii {
  float topLeft = 0.0f;
  float topRight = 0.0f;
  float bottomRight = 0.0f;
  float bottomLeft = 0.0f;
};

// Extra space, in logical px, that the shadow bleeds past each edge of the
// element box. Computed on the JS side and passed in verbatim so the native
// view size and the rasterized buffer size always agree.
struct Bleed {
  float left = 0.0f;
  float top = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;
};

// RGBA8888, premultiplied alpha, row-major, tightly packed (stride = width * 4).
struct Bitmap {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;

  bool empty() const {
    return width <= 0 || height <= 0 ||
           pixels.size() != static_cast<size_t>(width) * height * 4;
  }
};

struct RenderRequest {
  float contentWidth = 0.0f;   // logical px, the element box
  float contentHeight = 0.0f;  // logical px
  CornerRadii radii;
  Bleed bleed;
  float scale = 1.0f;  // device pixel ratio
  bool highQuality = false;  // use the slower exact rounded-corner quadrature
  std::vector<ShadowLayer> layers;
};

}  // namespace figmashadow
