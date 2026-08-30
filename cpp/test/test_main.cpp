// Standalone test / sanity harness for the shared C++ core.
//
//   c++ -std=c++17 -O2 -I../figmashadow test_main.cpp ../figmashadow/*.cpp -o /tmp/fstest && /tmp/fstest
//
// Exits non-zero on failure. Also writes a few PPMs to /tmp for eyeballing.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

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
                    "0px 0px 20px 0px rgba(0,0,0,0.5)",
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
  Bitmap o = render(120, 80, 16, 16, 16, 16, "8px 10px 16px 0px rgba(0,0,0,0.6)",
                    40, 40, 40, 40, 2.0f);
  CHECK(!o.empty());
  writePPM(o, "/tmp/fs_offset.ppm");
  float br = alphaAt(o, o.width * 3 / 4, o.height * 3 / 4);
  float tl = alphaAt(o, o.width / 4, o.height / 4);
  CHECK(br > tl);

  // Inset shadow: darker near the top edge (shadow cast downward), clear in the
  // middle, nothing outside the element.
  Bitmap in = render(140, 100, 10, 10, 10, 10, "inset 0px 8px 12px 0px rgba(0,0,0,0.7)",
                     0, 0, 0, 0, 2.0f);
  CHECK(!in.empty());
  CHECK(in.width == 280 && in.height == 200);
  writePPM(in, "/tmp/fs_inset.ppm");
  float topBand = alphaAt(in, in.width / 2, 8);
  float centre = alphaAt(in, in.width / 2, in.height / 2);
  CHECK(topBand > 0.15f);
  CHECK(centre < 0.05f);

  // Multiple shadows compose and stay within [0,1].
  Bitmap m = render(100, 100, 8, 8, 8, 8,
                    "0 1px 2px rgba(0,0,0,0.2), 0 8px 24px rgba(0,0,0,0.25)",
                    40, 40, 40, 40, 1.0f);
  CHECK(!m.empty());
  for (size_t i = 3; i < m.pixels.size(); i += 4) CHECK(m.pixels[i] <= 255);

  // Cache returns an identical buffer.
  Bitmap m2 = render(100, 100, 8, 8, 8, 8,
                     "0 1px 2px rgba(0,0,0,0.2), 0 8px 24px rgba(0,0,0,0.25)",
                     40, 40, 40, 40, 1.0f);
  CHECK(m2.pixels == m.pixels);
  CHECK(cacheSizeBytes() > 0);

  std::printf("  wrote /tmp/fs_drop.ppm /tmp/fs_offset.ppm /tmp/fs_inset.ppm\n");
}

static void testDeterminism() {
  std::printf("determinism\n");
  clearCache();
  Bitmap a = render(83.5f, 47.25f, 9, 3, 21, 0, "3px -5px 17px 2px rgba(10,20,30,0.44)",
                    30, 30, 30, 30, 3.0f);
  clearCache();
  Bitmap b = render(83.5f, 47.25f, 9, 3, 21, 0, "3px -5px 17px 2px rgba(10,20,30,0.44)",
                    30, 30, 30, 30, 3.0f);
  CHECK(a.pixels == b.pixels);
}

int main() {
  testColor();
  testParser();
  testRasterizer();
  testDeterminism();
  if (g_failures == 0) {
    std::printf("\nOK — all checks passed\n");
    return 0;
  }
  std::printf("\n%d check(s) failed\n", g_failures);
  return 1;
}
