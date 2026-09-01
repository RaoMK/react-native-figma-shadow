#include "Rasterizer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace figmashadow {

namespace {

constexpr float kSqrt2 = 1.41421356237f;
constexpr float kInvSqrt2Pi = 0.39894228040f;
constexpr int kMaxDimension = 8192;

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}
inline float gauss1d(float d, float sigma) {
  return std::exp(-(d * d) / (2.0f * sigma * sigma)) * kInvSqrt2Pi / sigma;
}

struct Radii4 {
  float tl, tr, br, bl;
};

// Signed distance to a rounded rectangle centred at the origin with half-extents
// (hx, hy). Negative inside.
float sdRoundRect(float x, float y, float hx, float hy, const Radii4& r) {
  float rr = (x >= 0.0f) ? ((y >= 0.0f) ? r.br : r.tr) : ((y >= 0.0f) ? r.bl : r.tl);
  rr = clampf(rr, 0.0f, std::min(hx, hy));
  float qx = std::fabs(x) - (hx - rr);
  float qy = std::fabs(y) - (hy - rr);
  float ax = std::max(qx, 0.0f);
  float ay = std::max(qy, 0.0f);
  float outside = std::sqrt(ax * ax + ay * ay);
  float inside = std::min(std::max(qx, qy), 0.0f);
  return outside + inside - rr;
}

// Antialiased hard coverage of the rounded rectangle (used for clipping and for
// the element knock-out).
float hardCoverage(float x, float y, float hx, float hy, const Radii4& r, float aa) {
  float sd = sdRoundRect(x, y, hx, hy, r);
  return clamp01(0.5f - sd / std::max(aa, 1e-4f));
}

// Exact Gaussian convolution of a sharp-cornered rectangle: the 2D integral is
// separable into a product of error functions.
float rectCoverage(float x, float y, float hx, float hy, float sigma) {
  float s2 = kSqrt2 * sigma;
  float gx = 0.5f * (std::erf((hx - x) / s2) + std::erf((hx + x) / s2));
  float gy = 0.5f * (std::erf((hy - y) / s2) + std::erf((hy + y) / s2));
  return clamp01(gx * gy);
}

// Half-width of the shape, measured from the vertical centre line, at signed
// vertical coordinate `t`. `rNeg` is the corner radius used for t <= 0, `rPos`
// for t > 0.
float sideExtent(float t, float hx, float hy, float rNeg, float rPos) {
  float r = (t <= 0.0f) ? rNeg : rPos;
  r = clampf(r, 0.0f, std::min(hx, hy));
  float at = std::fabs(t);
  if (at >= hy) return 0.0f;
  float straight = hy - r;
  if (at <= straight) return hx;
  float dy = at - straight;
  return (hx - r) + std::sqrt(std::max(0.0f, r * r - dy * dy));
}

// Exact-ish Gaussian convolution of a rounded rectangle via bounded vertical
// quadrature. Slower; used only when `highQuality` is requested.
float quadratureCoverage(float x, float y, float hx, float hy, const Radii4& r,
                         float sigma) {
  float s2 = kSqrt2 * sigma;
  float R = 3.5f * sigma;
  int n = static_cast<int>(std::ceil(9.5f * sigma / 0.75f));
  n = std::min(std::max(n, 16), 160);
  float step = (2.0f * R) / n;
  float lo = y - R;
  float acc = 0.0f;
  float wsum = 0.0f;
  for (int i = 0; i < n; ++i) {
    float t = lo + (i + 0.5f) * step;
    float w = gauss1d(y - t, sigma);
    float re = sideExtent(t, hx, hy, r.tr, r.br);
    float le = sideExtent(t, hx, hy, r.tl, r.bl);
    float horiz = 0.0f;
    if (re > 0.0f || le > 0.0f) {
      horiz = clamp01(0.5f * (std::erf((re - x) / s2) + std::erf((le + x) / s2)));
    }
    acc += w * horiz;
    wsum += w;
  }
  return (wsum > 1e-6f) ? clamp01(acc / wsum) : 0.0f;
}

// Gaussian convolution of a rounded rectangle. Fast path: exact for sharp
// corners, an SDF/error-function approximation for rounded corners (visually
// indistinguishable for shadows and, crucially, identical on every platform).
float blurredCoverage(float x, float y, float hx, float hy, const Radii4& r,
                      float sigma, float aa, bool highQuality) {
  if (hx <= 0.0f || hy <= 0.0f) return 0.0f;
  if (sigma < 0.6f) return hardCoverage(x, y, hx, hy, r, std::max(aa, 0.75f));
  if (r.tl == 0.0f && r.tr == 0.0f && r.br == 0.0f && r.bl == 0.0f) {
    return rectCoverage(x, y, hx, hy, sigma);
  }
  if (highQuality) return quadratureCoverage(x, y, hx, hy, r, sigma);
  float sd = sdRoundRect(x, y, hx, hy, r);
  return clamp01(0.5f * (1.0f + std::erf(-sd / (kSqrt2 * sigma))));
}

Radii4 spreadRadii(const CornerRadii& r, float spread, float hx, float hy) {
  auto grow = [&](float v) {
    float out = (v > 0.0f) ? std::max(0.0f, v + spread) : 0.0f;
    return clampf(out, 0.0f, std::min(hx, hy));
  };
  return {grow(r.topLeft), grow(r.topRight), grow(r.bottomRight), grow(r.bottomLeft)};
}

Radii4 shrinkRadii(const CornerRadii& r, float spread, float hx, float hy) {
  auto shrink = [&](float v) {
    float out = (v > 0.0f) ? std::max(0.0f, v - spread) : 0.0f;
    return clampf(out, 0.0f, std::min(std::max(hx, 0.0f), std::max(hy, 0.0f)));
  };
  return {shrink(r.topLeft), shrink(r.topRight), shrink(r.bottomRight), shrink(r.bottomLeft)};
}

// Premultiplied float RGBA accumulation buffer.
struct FImage {
  int w = 0;
  int h = 0;
  std::vector<float> px;  // w*h*4
};

inline void compositeOver(float* dst, float sr, float sg, float sb, float sa) {
  float inv = 1.0f - sa;
  dst[0] = sr + dst[0] * inv;
  dst[1] = sg + dst[1] * inv;
  dst[2] = sb + dst[2] * inv;
  dst[3] = sa + dst[3] * inv;
}

}  // namespace

Bitmap renderShadow(const RenderRequest& req) {
  Bitmap result;

  const float logicalW = req.bleed.left + req.contentWidth + req.bleed.right;
  const float logicalH = req.bleed.top + req.contentHeight + req.bleed.bottom;
  const bool nothingToDraw =
      req.layers.empty() && !(req.hasFill && req.fill.a > 0.0f);
  if (logicalW <= 0.0f || logicalH <= 0.0f || nothingToDraw) return result;

  const float scale = std::max(req.scale, 0.1f);
  int outW = std::min(kMaxDimension,
                      std::max(1, static_cast<int>(std::lround(logicalW * scale))));
  int outH = std::min(kMaxDimension,
                      std::max(1, static_cast<int>(std::lround(logicalH * scale))));

  // Drop to a lower internal resolution for pathologically large surfaces; the
  // factor is derived purely from the inputs, so both platforms pick the same
  // one.
  double outPx = static_cast<double>(outW) * outH;
  float internalScale = scale;
  if (outPx > kMaxRenderPixels) {
    internalScale = scale * static_cast<float>(std::sqrt(kMaxRenderPixels / outPx));
  }
  int rw = std::max(1, static_cast<int>(std::lround(logicalW * internalScale)));
  int rh = std::max(1, static_cast<int>(std::lround(logicalH * internalScale)));
  const float aa = 1.0f / internalScale;

  FImage acc;
  acc.w = rw;
  acc.h = rh;
  acc.px.assign(static_cast<size_t>(rw) * rh * 4, 0.0f);

  const float hx = req.contentWidth * 0.5f;
  const float hy = req.contentHeight * 0.5f;
  const float ecx = req.bleed.left + hx;
  const float ecy = req.bleed.top + hy;
  const Radii4 elementRadii{req.radii.topLeft, req.radii.topRight,
                            req.radii.bottomRight, req.radii.bottomLeft};

  // Paint order (bottom to top): drop shadows, then the fill, then inset
  // shadows. Within each shadow group the first layer in the list ends up on
  // top, so iterate the list in reverse.
  for (int pass = 0; pass < 3; ++pass) {
    if (pass == 1) {
      // --- fill pass ---
      if (!req.hasFill || req.fill.a <= 0.0f) continue;
      const Color& f = req.fill;
      for (int py = 0; py < rh; ++py) {
        const float ly = (py + 0.5f) / internalScale;
        float* row = acc.px.data() + static_cast<size_t>(py) * rw * 4;
        for (int px = 0; px < rw; ++px) {
          const float lx = (px + 0.5f) / internalScale;
          float cov = hardCoverage(lx - ecx, ly - ecy, hx, hy, elementRadii, aa);
          float a = f.a * cov;
          if (a <= 0.001f) continue;
          compositeOver(row + px * 4, f.r * a, f.g * a, f.b * a, a);
        }
      }
      continue;
    }

    const bool wantInset = (pass == 2);

  for (auto it = req.layers.rbegin(); it != req.layers.rend(); ++it) {
    const ShadowLayer& layer = *it;
    if (layer.color.a <= 0.0f || layer.inset != wantInset) continue;
    const float sigma = layer.blur * 0.5f;

    if (!layer.inset) {
      const float shx = hx + layer.spread;
      const float shy = hy + layer.spread;
      if (shx <= 0.0f || shy <= 0.0f) continue;
      const Radii4 sr = spreadRadii(req.radii, layer.spread, shx, shy);
      const float cxr = ecx + layer.offsetX;
      const float cyr = ecy + layer.offsetY;

      for (int py = 0; py < rh; ++py) {
        const float ly = (py + 0.5f) / internalScale;
        float* row = acc.px.data() + static_cast<size_t>(py) * rw * 4;
        for (int px = 0; px < rw; ++px) {
          const float lx = (px + 0.5f) / internalScale;
          float cov = blurredCoverage(lx - cxr, ly - cyr, shx, shy, sr, sigma, aa,
                                      req.highQuality);
          if (cov <= 0.001f) continue;
          float knock = hardCoverage(lx - ecx, ly - ecy, hx, hy, elementRadii, aa);
          float a = layer.color.a * cov * (1.0f - knock);
          if (a <= 0.001f) continue;
          compositeOver(row + px * 4, layer.color.r * a, layer.color.g * a,
                        layer.color.b * a, a);
        }
      }
    } else {
      const float ihx = hx - layer.spread;
      const float ihy = hy - layer.spread;
      const Radii4 ir = shrinkRadii(req.radii, layer.spread, ihx, ihy);
      const float cxr = ecx + layer.offsetX;
      const float cyr = ecy + layer.offsetY;
      const bool collapsed = ihx <= 0.0f || ihy <= 0.0f;

      for (int py = 0; py < rh; ++py) {
        const float ly = (py + 0.5f) / internalScale;
        float* row = acc.px.data() + static_cast<size_t>(py) * rw * 4;
        for (int px = 0; px < rw; ++px) {
          const float lx = (px + 0.5f) / internalScale;
          float inside = hardCoverage(lx - ecx, ly - ecy, hx, hy, elementRadii, aa);
          if (inside <= 0.001f) continue;
          float innerCov =
              collapsed ? 0.0f
                        : blurredCoverage(lx - cxr, ly - cyr, ihx, ihy, ir, sigma,
                                          aa, req.highQuality);
          float a = layer.color.a * (1.0f - innerCov) * inside;
          if (a <= 0.001f) continue;
          compositeOver(row + px * 4, layer.color.r * a, layer.color.g * a,
                        layer.color.b * a, a);
        }
      }
    }
  }
  }  // pass loop

  // Resample (identity when rw==outW && rh==outH) and pack to premultiplied
  // RGBA8888.
  result.width = outW;
  result.height = outH;
  result.pixels.assign(static_cast<size_t>(outW) * outH * 4, 0);
  for (int oy = 0; oy < outH; ++oy) {
    float fy = (oy + 0.5f) * rh / outH - 0.5f;
    int y0 = static_cast<int>(std::floor(fy));
    float wy = fy - y0;
    int y0c = std::min(std::max(y0, 0), rh - 1);
    int y1c = std::min(std::max(y0 + 1, 0), rh - 1);
    for (int ox = 0; ox < outW; ++ox) {
      float fx = (ox + 0.5f) * rw / outW - 0.5f;
      int x0 = static_cast<int>(std::floor(fx));
      float wx = fx - x0;
      int x0c = std::min(std::max(x0, 0), rw - 1);
      int x1c = std::min(std::max(x0 + 1, 0), rw - 1);

      const float* p00 = acc.px.data() + (static_cast<size_t>(y0c) * rw + x0c) * 4;
      const float* p10 = acc.px.data() + (static_cast<size_t>(y0c) * rw + x1c) * 4;
      const float* p01 = acc.px.data() + (static_cast<size_t>(y1c) * rw + x0c) * 4;
      const float* p11 = acc.px.data() + (static_cast<size_t>(y1c) * rw + x1c) * 4;

      uint8_t* out = result.pixels.data() + (static_cast<size_t>(oy) * outW + ox) * 4;
      for (int c = 0; c < 4; ++c) {
        float top = p00[c] * (1.0f - wx) + p10[c] * wx;
        float bot = p01[c] * (1.0f - wx) + p11[c] * wx;
        float v = top * (1.0f - wy) + bot * wy;
        out[c] = static_cast<uint8_t>(std::lround(clamp01(v) * 255.0f));
      }
    }
  }
  return result;
}

}  // namespace figmashadow
