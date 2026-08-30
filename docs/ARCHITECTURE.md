# Architecture

## The parity guarantee

Every pixel of every shadow is produced by one function — `figmashadow::renderShadow`
in [`cpp/figmashadow/Rasterizer.cpp`](../cpp/figmashadow/Rasterizer.cpp) — compiled
once from the same source into both the iOS pod and the Android `.so`. Neither
platform runs any blur code of its own (`CALayer.shadowRadius`, `elevation`,
`RenderEffect`, SVG `feGaussianBlur`, …), so there is nothing to diverge.

The math is an analytic Gaussian convolution of a rounded rectangle:

- **Sharp corners** (all radii 0): the 2D integral is separable into a product
  of error functions — exact and cheap.
- **Rounded corners** (fast path, default): an SDF/error-function approximation.
  Visually indistinguishable for shadows; ~1 `erf` per pixel.
- **Rounded corners** (`highQuality`): a bounded vertical quadrature of the
  horizontal error-function term. Slower, closer to a reference `box-shadow`.

All three are pure `float`/`double` arithmetic — deterministic under IEEE-754,
so the same inputs give the same bytes on every device.

## Data flow

```
<Shadow shadow="0 4px 20px rgba(0,0,0,.15)" borderRadius={16}>
        │
        │  src/parseShadow.ts  — JS parses only enough to size the bleed
        │  src/index.tsx       — reserves the bleed with negative margins
        ▼
FigmaShadowView  (Fabric native component, codegen spec in
                  src/FigmaShadowViewNativeComponent.ts)
        │  props: shadow string, content size, radii, bleed, pixelRatio
        ▼
figmashadow::render(...)          cpp/figmashadow/FigmaShadow.cpp
        │  parse (Parser.cpp) → rasterize (Rasterizer.cpp) → LRU memoize
        ▼
premultiplied RGBA8888 bitmap, (content + bleed) * scale device px
        │
   ┌────┴─────────────────────────────┐
   ▼                                  ▼
iOS: CGImage → CALayer.contents     Android: Bitmap → Canvas.drawBitmap
     (ios/FigmaShadowView.mm)             (android/.../FigmaShadowView.kt)
```

Rasterization runs on a background thread on both platforms; the memo cache
(keyed on the quantised request) makes every repeat — e.g. every row of a list —
effectively free.

## Layout model

`<Shadow>` renders an outer `View` (your layout box) wrapping the native view.
The native view's border-box is `content + 2 × bleed`; equal **negative margins**
pull its frame back so siblings lay out as if only the children were there, and
equal **padding** pushes the children into the centre region. The shadow paints
into the bleed band, which is inside the native view's own bounds — so nothing
depends on `overflow` except an ancestor that sets `overflow: hidden`, which
clips the shadow exactly as CSS would.

## What is not done yet

- No runnable example app in this repo (planned).
- `drop-shadow` that follows arbitrary content alpha (text, transparent PNGs) —
  only shape shadows derived from `borderRadius`.
- No animated/interpolatable shadow props.
- The `backgroundColor` fill uses one corner radius for all four when per-corner
  radii differ (the shadow itself is always exact).
