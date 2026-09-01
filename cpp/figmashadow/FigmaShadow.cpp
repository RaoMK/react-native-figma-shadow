#include "FigmaShadow.h"

#include <cmath>
#include <cstdint>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Parser.h"
#include "Rasterizer.h"

namespace figmashadow {

namespace {

// Quantise floats to 1/8 px before hashing so sub-pixel layout jitter still
// hits the cache.
long quant(float v) { return std::lround(v * 8.0f); }

std::string makeKey(float cw, float ch, float rtl, float rtr, float rbr, float rbl,
                    const std::string& boxShadow, const std::string& fillColor,
                    float bl, float bt, float br, float bb, float scale, bool hq) {
  std::ostringstream os;
  os << quant(cw) << '|' << quant(ch) << '|' << quant(rtl) << '|' << quant(rtr)
     << '|' << quant(rbr) << '|' << quant(rbl) << '|' << quant(bl) << '|'
     << quant(bt) << '|' << quant(br) << '|' << quant(bb) << '|' << quant(scale)
     << '|' << (hq ? '1' : '0') << '|' << fillColor << '|' << boxShadow;
  return os.str();
}

constexpr size_t kMaxCacheBytes = 6 * 1024 * 1024;

struct Cache {
  std::mutex mutex;
  std::list<std::pair<std::string, Bitmap>> items;  // front = most recent
  std::unordered_map<std::string, decltype(items)::iterator> index;
  size_t bytes = 0;

  bool get(const std::string& key, Bitmap& out) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = index.find(key);
    if (it == index.end()) return false;
    items.splice(items.begin(), items, it->second);
    out = it->second->second;
    return true;
  }

  void put(const std::string& key, const Bitmap& bmp) {
    std::lock_guard<std::mutex> lock(mutex);
    if (index.count(key)) return;
    items.emplace_front(key, bmp);
    index[key] = items.begin();
    bytes += bmp.pixels.size();
    while (bytes > kMaxCacheBytes && items.size() > 1) {
      auto& back = items.back();
      bytes -= back.second.pixels.size();
      index.erase(back.first);
      items.pop_back();
    }
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex);
    items.clear();
    index.clear();
    bytes = 0;
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(mutex);
    return bytes;
  }
};

Cache& cache() {
  static Cache instance;
  return instance;
}

}  // namespace

Bitmap render(float contentWidth, float contentHeight, float radiusTopLeft,
              float radiusTopRight, float radiusBottomRight, float radiusBottomLeft,
              const std::string& boxShadow, const std::string& fillColor,
              float bleedLeft, float bleedTop, float bleedRight, float bleedBottom,
              float scale, bool highQuality) {
  const std::string key =
      makeKey(contentWidth, contentHeight, radiusTopLeft, radiusTopRight,
              radiusBottomRight, radiusBottomLeft, boxShadow, fillColor, bleedLeft,
              bleedTop, bleedRight, bleedBottom, scale, highQuality);

  Bitmap cached;
  if (cache().get(key, cached)) return cached;

  RenderRequest req;
  req.contentWidth = contentWidth;
  req.contentHeight = contentHeight;
  req.radii = {radiusTopLeft, radiusTopRight, radiusBottomRight, radiusBottomLeft};
  req.bleed = {bleedLeft, bleedTop, bleedRight, bleedBottom};
  req.scale = scale;
  req.highQuality = highQuality;
  req.hasFill = !fillColor.empty() && parseColor(fillColor, req.fill);
  req.layers = parseBoxShadow(boxShadow);

  Bitmap bmp = renderShadow(req);
  if (!bmp.empty()) cache().put(key, bmp);
  return bmp;
}

void clearCache() { cache().clear(); }

size_t cacheSizeBytes() { return cache().size(); }

}  // namespace figmashadow
