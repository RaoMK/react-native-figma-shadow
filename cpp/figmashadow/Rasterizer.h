#pragma once

#include "Types.h"

namespace figmashadow {

// Renders every layer of a box-shadow into a single premultiplied RGBA8888
// bitmap sized to (content + bleed) * scale device pixels.
//
// The math is identical on every platform: an analytic Gaussian convolution of a
// rounded rectangle (exact error-function form for the sharp-cornered case, a
// bounded vertical quadrature for rounded corners). No platform 2D library is
// involved, so iOS and Android produce the same bytes.
Bitmap renderShadow(const RenderRequest& req);

// Largest render buffer, in device pixels, before the rasterizer transparently
// drops to a lower internal resolution (the result is still returned at full
// size via nearest sizing metadata on the caller side — here we simply cap).
constexpr double kMaxRenderPixels = 4.0e6;

}  // namespace figmashadow
