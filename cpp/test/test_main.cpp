// Standalone test / sanity harness for the shared C++ core.
//
//   c++ -std=c++17 -O2 -I../figmashadow test_main.cpp ../figmashadow/*.cpp -o /tmp/fstest && /tmp/fstest
//
// Exits non-zero on failure. Also writes a few PPMs to /tmp for eyeballing.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "Color.h"
#include "FigmaShadow.h"
#include "Parser.h"
#include "Rasterizer.h"

using namespace figmashadow;

static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);       \
      ++g_failures;                                                       \
    }                                                                    \
  } while (0)

static bool approx(float a, float b, float eps = 1e-3f) {
  return std::fabs(a - b) <= eps;
}

static void testColor() {
  std::printf("color\n");
  Color c;
  CHECK(parseColor("#000", c) && approx(c.r, 0) && approx(c.a, 1));
  CHECK(parseColor("#ffffff", c) && approx(c.r, 1) && approx(c.g, 1) && approx(c.b, 1));
  CHECK(parseColor("#ff000080", c) && approx(c.r, 1) && approx(c.a, 128.0f / 255.0f));
  CHECK(parseColor("rgba(0, 0, 0, 0.5)", c) && approx(c.a, 0.5f) && approx(c.r, 0));
  CHECK(parseColor("rgb(255 128 0)", c) && approx(c.r, 1) && approx(c.g, 128.0f / 255.0f));
  CHECK(parseColor("rgba(0 0 0 / 25%)", c) && approx(c.a, 0.25f));
  CHECK(parseColor("hsl(0, 100%, 50%)", c) && approx(c.r, 1) && approx(c.g, 0) && approx(c.b, 0));
  CHECK(parseColor("black", c) && approx(c.r, 0) && approx(c.a, 1));
  CHECK(parseColor("transparent", c) && approx(c.a, 0));
  CHECK(!parseColor("0", c));
  CHECK(!parseColor("4px", c));
  CHECK(!parseColor("notacolor", c));
}

static void testParser() {
  std::printf("parser\n");

  auto a = parseBoxShadow("0px 4px 20px 0px rgba(0, 0, 0, 0.15)");
  CHECK(a.size() == 1);
  if (a.size() == 1) {
    CHECK(approx(a[0].offsetX, 0));
    CHECK(approx(a[0].offsetY, 4));
    CHECK(approx(a[0].blur, 20));
    CHECK(approx(a[0].spread, 0));
    CHECK(approx(a[0].color.a, 0.15f));
    CHECK(!a[0].inset);
  }

  auto b = parseBoxShadow("box-shadow: 0 2px 4px rgba(0,0,0,.1), 0 12px 32px rgba(0,0,0,.14);");
  CHECK(b.size() == 2);
  if (b.size() == 2) {
    CHECK(approx(b[0].offsetY, 2));
    CHECK(approx(b[1].blur, 32));
    CHECK(approx(b[1].color.a, 0.14f));
  }

  auto c = parseBoxShadow("inset 0 2px 8px rgba(0,0,0,0.25)");
  CHECK(c.size() == 1 && c[0].inset && approx(c[0].blur, 8));

  auto d = parseBoxShadow("rgba(0,0,0,0.3) 0 4px 8px 2px inset");
  CHECK(d.size() == 1 && d[0].inset && approx(d[0].spread, 2) && approx(d[0].color.a, 0.3f));

  auto e = parseBoxShadow("2px 2px #ff0000");
  CHECK(e.size() == 1 && approx(e[0].blur, 0) && approx(e[0].color.r, 1));

  CHECK(parseBoxShadow("").empty());
  CHECK(parseBoxShadow("none").empty());
  CHECK(parseBoxShadow("garbage").empty());
}

static float alphaAt(const Bitmap& b, int x, int y) {
  if (x < 0 || y < 0 || x >= b.width || y >= b.height) return -1;
  return b.pixels[(static_cast<size_t>(y) * b.width + x) * 4 + 3] / 255.0f;
}

static void writePPM(const Bitmap& b, const std::string& path) {
  std::ofstream f(path, std::ios::binary);
  f << "P6\n" << b.width << " " << b.height << "\n255\n";
  for (int i = 0; i < b.width * b.height; ++i) {
    // un-premultiply against white so the PPM reads like a screenshot on paper
    float a = b.pixels[i * 4 + 3] / 255.0f;
    for (int c = 0; c < 3; ++c) {
      float pm = b.pixels[i * 4 + c] / 255.0f;
      float v = pm + (1.0f - a);  // over white
      unsigned char u = static_cast<unsigned char>(std::lround(std::min(1.0f, v) * 255.0f));
      f.put(static_cast<char>(u));
    }
  }
}

static void testRasterizer() {
  std::printf("rasterizer\n");

  // A centred 100x60 card, radius 12, single soft drop shadow, no offset.
  float bleedEach = std::ceil(1.5f * 20.0f + 0.0f) + 1.0f;  // ~31
  Bitmap b = render(/*cw*/ 100, /*ch*/ 60, 12, 12, 12, 12,
                    "0px 0px 20px 0px rgba(0,0,0,0.5)", "",
                    bleedEach, bleedEach, bleedEach, bleedEach, /*scale*/ 2.0f);
  CHECK(!b.empty());
  CHECK(b.width == static_cast<int>(std::lround((100 + 2 * bleedEach) * 2)));
  writePPM(b, "/tmp/fs_drop.ppm");

  // Centre pixel sits under the (opaque-knockout) element: alpha ~ 0.
  float ca = alphaAt(b, b.width / 2, b.height / 2);
  CHECK(ca >= 0 && ca < 0.05f);

  // Symmetry: left/right and top/bottom mirror for a centred, un-offset shadow.
  for (int probe : {10, 25, 40}) {
    float l = alphaAt(b, probe, b.height / 2);
    float r = alphaAt(b, b.width - 1 - probe, b.height / 2);
    CHECK(approx(l, r, 0.02f));
    float t = alphaAt(b, b.width / 2, probe);
    float bo = alphaAt(b, b.width / 2, b.height - 1 - probe);
    CHECK(approx(t, bo, 0.02f));
  }

  // Along the mid row, from the element's right edge the shadow peaks and then
  // falls off monotonically to zero.
  int midY = b.height / 2;
  int peakX = b.width / 2;
  float peak = 0.0f;
  for (int x = b.width / 2; x < b.width; ++x) {
    float v = alphaAt(b, x, midY);
    if (v > peak) { peak = v; peakX = x; }
  }
  CHECK(peak > 0.2f);
  bool monotone = true;
  float prev = 2.0f;
  for (int x = peakX; x < b.width; ++x) {
    float v = alphaAt(b, x, midY);
    if (v > prev + 0.02f) monotone = false;
    prev = v;
  }
  CHECK(monotone);

  // Corners of the canvas are far from the shadow -> fully transparent.
  CHECK(alphaAt(b, 0, 0) < 0.02f);

  // Offset shadow: more shadow below-right than above-left.
  Bitmap o = render(120, 80, 16, 16, 16, 16, "8px 10px 16px 0px rgba(0,0,0,0.6)", "",
                    40, 40, 40, 40, 2.0f);
  CHECK(!o.empty());
  writePPM(o, "/tmp/fs_offset.ppm");
  float br = alphaAt(o, o.width * 3 / 4, o.height * 3 / 4);
  float tl = alphaAt(o, o.width / 4, o.height / 4);
  CHECK(br > tl);

  // Inset shadow: darker near the top edge (shadow cast downward), clear in the
  // middle, nothing outside the element.
  Bitmap in = render(140, 100, 10, 10, 10, 10, "inset 0px 8px 12px 0px rgba(0,0,0,0.7)", "",
                     0, 0, 0, 0, 2.0f);
  CHECK(!in.empty());
  CHECK(in.width == 280 && in.height == 200);
  writePPM(in, "/tmp/fs_inset.ppm");
  float topBand = alphaAt(in, in.width / 2, 8);
  float centre = alphaAt(in, in.width / 2, in.height / 2);
  CHECK(topBand > 0.15f);
  CHECK(centre < 0.05f);

  // Inset shadow WITH a fill: the bitmap is now fully opaque over the element
  // (fill baked in), darker at the top edge, and the fill colour shows through
  // in the centre.
  Bitmap inf = render(140, 100, 10, 10, 10, 10, "inset 0px 8px 12px 0px rgba(0,0,0,0.7)",
                      "#ffffff", 0, 0, 0, 0, 2.0f);
  CHECK(!inf.empty());
  auto px = [&](const Bitmap& bm, int x, int y, int c) {
    return bm.pixels[(static_cast<size_t>(y) * bm.width + x) * 4 + c] / 255.0f;
  };
  CHECK(px(inf, inf.width / 2, inf.height / 2, 3) > 0.98f);   // centre opaque
  CHECK(px(inf, inf.width / 2, inf.height / 2, 0) > 0.90f);   // centre ~white
  CHECK(px(inf, inf.width / 2, 6, 0) < 0.75f);                // top edge darkened

  // Multiple shadows compose; every pixel stays a valid premultiplied colour
  // (each of R, G, B <= A).
  Bitmap m = render(100, 100, 8, 8, 8, 8,
                    "0 1px 2px rgba(0,0,0,0.2), 0 8px 24px rgba(0,0,0,0.25)", "",
                    40, 40, 40, 40, 1.0f);
  CHECK(!m.empty());
  bool premultValid = true;
  for (size_t i = 0; i + 3 < m.pixels.size(); i += 4) {
    uint8_t a = m.pixels[i + 3];
    if (m.pixels[i] > a || m.pixels[i + 1] > a || m.pixels[i + 2] > a) premultValid = false;
  }
  CHECK(premultValid);

  // Cache returns an identical buffer.
  Bitmap m2 = render(100, 100, 8, 8, 8, 8,
                     "0 1px 2px rgba(0,0,0,0.2), 0 8px 24px rgba(0,0,0,0.25)", "",
                     40, 40, 40, 40, 1.0f);
  CHECK(m2.pixels == m.pixels);
  CHECK(cacheSizeBytes() > 0);

  std::printf("  wrote /tmp/fs_drop.ppm /tmp/fs_offset.ppm /tmp/fs_inset.ppm\n");
}

// --- ground-truth CSS box-shadow: supersampled rounded-rect mask convolved with
//     a true separable Gaussian (sigma = blur / 2) ---

static float sdRoundRect(float x, float y, float hx, float hy, float r) {
  r = std::min(r, std::min(std::max(hx, 0.0f), std::max(hy, 0.0f)));
  float qx = std::fabs(x) - (hx - r);
  float qy = std::fabs(y) - (hy - r);
  float ax = std::max(qx, 0.0f), ay = std::max(qy, 0.0f);
  return std::sqrt(ax * ax + ay * ay) + std::min(std::max(qx, qy), 0.0f) - r;
}

static std::vector<float> gaussianBlurredRoundRect(int W, int H, float cx, float cy,
                                                   float hx, float hy, float r,
                                                   float sigma) {
  const int SS = 3;
  const int hw = W * SS, hh = H * SS;
  std::vector<float> m(static_cast<size_t>(hw) * hh);
  for (int y = 0; y < hh; ++y)
    for (int x = 0; x < hw; ++x)
      m[static_cast<size_t>(y) * hw + x] =
          sdRoundRect((x + 0.5f) / SS - cx, (y + 0.5f) / SS - cy, hx, hy, r) < 0.0f ? 1.0f
                                                                                   : 0.0f;
  const float s = sigma * SS;
  const int kr = static_cast<int>(std::ceil(4.0f * s));
  std::vector<float> k(2 * kr + 1);
  float ksum = 0.0f;
  for (int i = -kr; i <= kr; ++i) {
    k[i + kr] = std::exp(-(i * i) / (2.0f * s * s));
    ksum += k[i + kr];
  }
  for (float& v : k) v /= ksum;
  std::vector<float> t(static_cast<size_t>(hw) * hh), o(static_cast<size_t>(hw) * hh);
  for (int y = 0; y < hh; ++y)
    for (int x = 0; x < hw; ++x) {
      float a = 0.0f;
      for (int i = -kr; i <= kr; ++i)
        a += m[static_cast<size_t>(y) * hw + std::min(std::max(x + i, 0), hw - 1)] * k[i + kr];
      t[static_cast<size_t>(y) * hw + x] = a;
    }
  for (int y = 0; y < hh; ++y)
    for (int x = 0; x < hw; ++x) {
      float a = 0.0f;
      for (int i = -kr; i <= kr; ++i)
        a += t[static_cast<size_t>(std::min(std::max(y + i, 0), hh - 1)) * hw + x] * k[i + kr];
      o[static_cast<size_t>(y) * hw + x] = a;
    }
  std::vector<float> out(static_cast<size_t>(W) * H, 0.0f);
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      float a = 0.0f;
      for (int sy = 0; sy < SS; ++sy)
        for (int sx = 0; sx < SS; ++sx)
          a += o[static_cast<size_t>(y * SS + sy) * hw + (x * SS + sx)];
      out[static_cast<size_t>(y) * W + x] = a / (SS * SS);
    }
  return out;
}

static void testAccuracyVsGaussian() {
  std::printf("accuracy vs true Gaussian\n");
  clearCache();

  const float W = 132, H = 84, radius = 16, factor = 2.0f;  // BLUR_EXTENT_FACTOR

  struct Case {
    const char* name;
    const char* shadow;
  };
  const Case cases[] = {
      {"soft drop", "0 0 20px 0 rgba(0,0,0,0.5)"},
      {"figma card", "0 4px 20px 0 rgba(0,0,0,0.15)"},
      {"offset", "8px 10px 16px 0 rgba(0,0,0,0.6)"},
      {"large", "0 8px 24px 0 rgba(0,0,0,0.2)"},
  };

  for (const auto& c : cases) {
    const auto layers = parseBoxShadow(c.shadow);
    const auto& L = layers[0];
    const float sigma = L.blur * 0.5f;
    const float ext = L.blur * factor + std::max(0.0f, L.spread);
    const float bl = std::ceil(ext + std::max(0.0f, -L.offsetX));
    const float br = std::ceil(ext + std::max(0.0f, L.offsetX));
    const float bt = std::ceil(ext + std::max(0.0f, -L.offsetY));
    const float bb = std::ceil(ext + std::max(0.0f, L.offsetY));

    Bitmap ours = render(W, H, radius, radius, radius, radius, c.shadow, "", bl, bt, br, bb, 1.0f);
    const int OW = ours.width, OH = ours.height;

    const float cx = bl + W * 0.5f + L.offsetX;
    const float cy = bt + H * 0.5f + L.offsetY;
    auto ref = gaussianBlurredRoundRect(OW, OH, cx, cy, W * 0.5f + L.spread,
                                        H * 0.5f + L.spread, radius, sigma);

    const float ecx = bl + W * 0.5f, ecy = bt + H * 0.5f;
    double sumErr = 0.0, refPeak = 0.0, ourPeak = 0.0;
    int n = 0;
    for (int y = 0; y < OH; ++y)
      for (int x = 0; x < OW; ++x) {
        float knock =
            sdRoundRect(x + 0.5f - ecx, y + 0.5f - ecy, W * 0.5f, H * 0.5f, radius) < -1.0f
                ? 1.0f
                : 0.0f;
        float refA = ref[static_cast<size_t>(y) * OW + x] * L.color.a * (1.0f - knock);
        float ourA = ours.pixels[(static_cast<size_t>(y) * OW + x) * 4 + 3] / 255.0f;
        sumErr += std::fabs(refA - ourA);
        refPeak = std::max(refPeak, static_cast<double>(refA));
        ourPeak = std::max(ourPeak, static_cast<double>(ourA));
        ++n;
      }
    double meanErr = sumErr / n;
    double peakRatio = ourPeak / refPeak;
    std::printf("  %-11s meanErr=%.4f  peak ours/ref=%.2f\n", c.name, meanErr, peakRatio);
    CHECK(meanErr < 0.015);
    CHECK(peakRatio > 0.80 && peakRatio < 1.20);
  }
}

static void testDeterminism() {
  std::printf("determinism\n");
  clearCache();
  Bitmap a = render(83.5f, 47.25f, 9, 3, 21, 0, "3px -5px 17px 2px rgba(10,20,30,0.44)", "",
                    30, 30, 30, 30, 3.0f);
  clearCache();
  Bitmap b = render(83.5f, 47.25f, 9, 3, 21, 0, "3px -5px 17px 2px rgba(10,20,30,0.44)", "",
                    30, 30, 30, 30, 3.0f);
  CHECK(a.pixels == b.pixels);
}

int main() {
  testColor();
  testParser();
  testRasterizer();
  testAccuracyVsGaussian();
  testDeterminism();
  if (g_failures == 0) {
    std::printf("\nOK — all checks passed\n");
    return 0;
  }
  std::printf("\n%d check(s) failed\n", g_failures);
  return 1;
}
