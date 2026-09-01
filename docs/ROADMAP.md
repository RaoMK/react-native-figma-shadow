# Roadmap

Direction, not dates. Priorities shift with feedback — open an issue if something
here matters to you or is missing.

## Now — `0.1.x` (stabilise)

Bug fixes and hardening on the current feature set. No breaking changes expected;
if one is unavoidable it ships in `0.2.0` with notes.

- Real-world soak in production apps; triage what comes back.
- **Device-matrix pass** — iPad, older iOS, Android API 24–35, common OEM skins,
  tablets, RTL, dark mode, unusual pixel densities.
- **Perf profiling** — `FlatList`/`FlashList` with hundreds of shadowed rows,
  view recycling, memory behaviour under cache pressure.
- **Edge cases** — `borderRadius` larger than half the box, negative-spread
  collapse, transformed / absolutely-positioned / zero-size children, nested
  `<Shadow>`, percentage dimensions, content that resizes after first paint.
- **First-frame flash** — a cheap synchronous path (or placeholder) for small
  shadows so nothing pops in a frame late.
- Clean up the `exports` / ESM story so the `builder-bob` warning goes away.

## `1.0.0` (commit to the API)

Ships when the list above is done, native CI has been green for a while, and the
package has real mileage.

- **Frozen, documented public API** with a stated stability guarantee.
- Native build tested on the full RN support range (0.76 → latest) in CI.
- A committed, runnable example app (not just a screen).

## After `1.0` — features

Roughly in the order they're likely to land.

- **`drop-shadow` mode** — the shadow follows the *actual alpha* of arbitrary
  content (text, icons, transparent PNGs), not just the `borderRadius`. Renders
  the child subtree to a bitmap, blurs, tints, composites behind. The most-asked
  gap.
- **Animated shadow props** — Reanimated integration: shared values driving
  blur / offset / spread / colour / opacity on the UI thread, fully
  interpolatable. Replaces today's "cross-fade two `<Shadow>` layers" workaround.
- **Design-token layer** — `<Shadow preset="md">`, built-in Material 3 / iOS /
  Tailwind elevation scales, theme-aware coloured shadows.
- **`react-native-web` target** — emit real CSS `box-shadow` on web so the same
  `<Shadow>` component is universal.
- **CSS `filter: drop-shadow()` syntax** in addition to `box-shadow`.
- **Continuous ("squircle") corners** to match iOS's smooth corner curve.

## Exploring — no commitment

- **Analytic composite path** — evaluate the shadow per-pixel in a shader at
  composite time for the common single-layer case, skipping the bitmap entirely.
- **Arbitrary shapes** — shadows for non-rectangular clip paths / SVG.
- **Nitro Modules** for the native bridge (less boilerplate, lower overhead).
- **Material presets** — glass, neumorphic, long-shadow.
- **Per-platform override** escape hatch for the rare case you *want* them to
  differ.

## Non-goals

- Matching React Native's own `boxShadow` output. Where they differ, this package
  aims to match a real browser / Figma; core RN's Android `boxShadow`
  over-spreads.
- A legacy-architecture (Paper) fallback.
