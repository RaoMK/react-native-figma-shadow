# How `react-native-figma-shadow` compares

Shadows in React Native have always been two half-solutions bolted together:
iOS has `shadowColor` / `shadowOffset` / `shadowOpacity` / `shadowRadius`;
Android has `elevation` (a fixed Material shadow you can't really shape or
colour). Every library below is an attempt to paper over that gap. Here is an
honest look at each, and where this package fits.

The one thing this package optimises for that nothing else does at the same time:
**a single rasterizer, so the output is byte-for-byte identical on both
platforms**, with **no large dependency** and **a memo cache** so a list of
identically-shadowed rows costs one bitmap.

---

## Summary table

| | figma-shadow | core `boxShadow` | shadow-2 | Skia `<Shadow>` | fast-shadow | drop-shadow |
|---|:---:|:---:|:---:|:---:|:---:|:---:|
| Pixel-identical iOS↔Android | ✅ | ⚠️ | ⚠️ | ✅ | ⚠️ | ❌ |
| Takes a CSS / Figma string | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| True Gaussian blur | ✅ | ✅ | ❌ (gradient) | ✅ | ✅ | ✅ |
| Multiple shadows | ✅ | ✅ | ❌ | ✅ | ❌ | ❌ |
| `inset` | ✅ | ✅ | ❌ | ✅ | ❌ | ❌ |
| spread | ✅ | ✅ | ✅ | ⚠️ | ❌ | ❌ |
| Follows content alpha (text, PNGs) | ❌ | ❌ | ❌ | ✅ | ❌ | ✅ (Android) |
| Peer dependencies | none | none | react-native-svg | @shopify/react-native-skia | none | none |
| Added download / arch | ~50–100 KB | 0 | ~0.5–1 MB | ~4–8 MB | ~10–20 KB | ~10–20 KB |
| Per-list bitmap cache | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Wraps a normal view (no `<Canvas>`) | ✅ | ✅ | ✅ | ❌ | ✅ | ✅ |
| Old architecture | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ |
| Actively maintained | new | ✅ (RN core) | intermittent | ✅ | low | low |

⚠️ = works, but the two platforms use different blur engines, so results are
*similar*, not identical.

---

## Core RN `boxShadow` (React Native 0.76+)

```tsx
<View style={{ boxShadow: '0 4px 20px rgba(0,0,0,0.15)' }} />
```

The New Architecture added a real CSS-ish styling layer: `boxShadow`, `filter:
[{ dropShadow }]`, `mixBlendMode`, `outline`. `boxShadow` takes the full CSS
grammar — multiple shadows, `inset`, spread, any colour — and it is **free and
built in**. If you can use it, start here.

**Where it falls short — and why this package exists:**

- **Not identical across platforms.** iOS maps `boxShadow` onto Core Animation
  layer shadows; Android draws its own `RenderNode`/outline-based shadow. Blur
  extent, edge falloff and corner behaviour differ visibly, especially at larger
  radii, and the Android path is newer and rougher.
- **No caching.** Every shadowed view is rendered independently and the string is
  re-parsed on every commit. A `FlatList` of 200 identical cards pays 200×.
- Same RN floor as this package (0.76+), so "works on older RN" is **not** a
  reason to pick `boxShadow` over it.

This package trades `boxShadow`'s zero cost for guaranteed fidelity plus the
cache, at ~50–100 KB and one native dependency.

## `@shopify/react-native-skia` — `<Shadow>` / `<BoxShadow>`

```tsx
<Canvas style={{ width, height }}>
  <RoundedRect x={0} y={0} width={W} height={H} r={16} color="white">
    <Shadow dx={0} dy={8} blur={20} color="rgba(0,0,0,0.15)" />
  </RoundedRect>
</Canvas>
```

Skia is one rendering engine compiled for both platforms, so — like this package
— its shadows **are** genuinely identical on iOS and Android, with a true
Gaussian, `inner` (inset), and multiple stacked `<Shadow>` children. It is the
most capable option and, if Skia is already in your app, essentially free at the
margin.

**Costs:**

- **~4–8 MB per architecture.** Pulling in the whole Skia runtime just for
  shadows is a lot.
- **Different paradigm.** You draw inside a `<Canvas>`; it does not transparently
  decorate an existing view subtree. Sizing, layout and content all move into
  Skia drawing ops or an absolutely-positioned overlay.
- No CSS-string input; no per-list caching of the raster.

Pick Skia if you already depend on it, or need shadows on arbitrary
Skia-drawn/Canvas content. Pick this package if you just want to decorate normal
RN views and don't want the megabytes.

## `react-native-shadow-2`

```tsx
<Shadow distance={20} startColor="rgba(0,0,0,0.15)" offset={[0, 4]}>
  <Card />
</Shadow>
```

The most-used community solution and the only one here that supports the **old
architecture**. It renders the shadow as an SVG (`react-native-svg` peer
dependency) using linear/radial **gradients**, not a Gaussian — so the falloff
curve doesn't match CSS or a real blur, particularly at large `distance`. No
multiple shadows, no `inset`. It often needs an explicit size or does a
measure pass that can flash for a frame. SVG rendering also has a per-shadow
cost.

Pick shadow-2 if you're still on the legacy architecture. Otherwise this package
gives you a real blur, CSS input, multi-shadow/inset, no `react-native-svg`, and
the cache.

## `react-native-fast-shadow`

Android draws the shadow with `Paint.setShadowLayer` on a rounded-rect
`Drawable`; iOS uses `CALayer.shadowPath`. It is **fast and tiny**. But: one
shadow only, one colour, **no spread, no inset**, values are a native "shadow
radius" rather than a CSS blur, and the two native blur engines are tuned to look
alike rather than being identical. Development activity is low.

Good when you want a cheap single drop shadow and don't care about exact parity.
This package is the choice when the shadow has to match a spec.

## `react-native-drop-shadow`

Android renders the child view to a bitmap and blurs **that**, so the shadow
follows the child's real alpha (text, transparent images) — its one genuine
advantage, shared only with Skia's `dropShadow`. But iOS just applies the
standard `CALayer` shadow props, so the two platforms are **not** consistent, and
the Android blur was built on **RenderScript, deprecated and removed in API 31+**.
Single shadow, single colour, legacy architecture.

Pick it only if you specifically need an Android drop shadow that hugs
non-rectangular content and can live with the platform mismatch. (Alpha-following
`drop-shadow` is on this package's roadmap; it is not in the current version.)

## Also seen in the wild

- **`react-native-shadow`** — the original SVG approach, unmaintained since ~2018;
  shadow-2 supersedes it.
- **`react-native-androw`** — Android colour shadow via RenderScript; deprecated.
- **NativeWind / Unistyles `shadow-*` utilities** — these just emit `boxShadow` /
  `elevation`, so they inherit core RN's behaviour and the platform mismatch
  above.

---

## Where this package is honestly weaker

- **No alpha-following `drop-shadow`.** Shadows follow the `borderRadius` you
  give, not arbitrary content shape. Skia and drop-shadow can do this today.
- **New Architecture only.** No RN &lt; 0.76 support at all.
- **Alpha maturity.** The shared C++ core is unit-tested and deterministic, but
  the iOS/Android native layers are young. shadow-2, Skia and core `boxShadow`
  are battle-tested.
- **A native dependency and ~50–100 KB.** Core `boxShadow` costs nothing.
