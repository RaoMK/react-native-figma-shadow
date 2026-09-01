# react-native-figma-shadow

[![npm](https://img.shields.io/npm/v/react-native-figma-shadow.svg)](https://www.npmjs.com/package/react-native-figma-shadow)
[![license](https://img.shields.io/npm/l/react-native-figma-shadow.svg)](./LICENSE)
[![New Architecture](https://img.shields.io/badge/New%20Architecture-required-blueviolet.svg)](https://reactnative.dev/architecture/landing-page)

Paste a CSS `box-shadow` — the exact string Figma's **Copy as CSS** produces, or
the one you already use on the web — and get a **pixel-identical** shadow on iOS
and Android.

```tsx
import { Shadow } from 'react-native-figma-shadow';

<Shadow shadow="0px 4px 20px 0px rgba(0, 0, 0, 0.15)" borderRadius={16}>
  <View style={styles.card} />
</Shadow>;
```

| `0 0 20px rgba(0,0,0,.5)` | `8px 10px 16px rgba(0,0,0,.6)` | `inset 0 8px 12px rgba(0,0,0,.7)` |
| :---: | :---: | :---: |
| ![drop](docs/images/fs_drop.png) | ![offset](docs/images/fs_offset.png) | ![inset](docs/images/fs_inset.png) |

<sub>Rendered by the shared C++ core — the byte-for-byte identical output feeds both the iOS <code>CALayer</code> and the Android <code>Canvas</code>.</sub>

> **0.1.0 — early but working.** The renderer is validated against a true Gaussian
> and covered by tests; the native layers have run on iOS and Android but not a
> wide device matrix. Expect the odd rough edge and possible API tweaks before
> 1.0. [Feedback and bug reports](https://github.com/RaoMK/react-native-figma-shadow/issues) very welcome.

## Highlights

- **One renderer, both platforms.** An analytic Gaussian convolution of a rounded
  rectangle, compiled once into a shared C++ core. iOS and Android run the same
  code and produce the same bytes — no `CALayer` blur vs `elevation` guesswork.
- **Spec-accurate.** Within ~5–8% of an ideal CSS `box-shadow` at the peak,
  near-exact in the body (guarded by a test against a reference Gaussian).
- **Zero peer dependencies.** No Skia, no `react-native-svg`. Adds ~50–100 KB to
  the app download.
- **Real CSS semantics.** Multiple comma-separated shadows, `inset`, spread,
  negative spread, per-corner radii, and any CSS color (`#rgb`, `#rrggbbaa`,
  `rgb()/rgba()`, `hsl()/hsla()`, named colors, `transparent`).
- **List-friendly.** Identical shadows (e.g. every row of a `FlatList`) are
  rasterized once and reused.

## Requirements

| | |
| --- | --- |
| React Native | **0.76+** with the **New Architecture** enabled (the default on 0.76+). No legacy-architecture fallback. |
| iOS | 13.4+ |
| Android | minSdkVersion 24+ |
| Expo | Works in a **development build** / prebuild (it is a native module — not available in Expo Go). |

## Install

```sh
npm install react-native-figma-shadow
# or
yarn add react-native-figma-shadow
```

```sh
cd ios && pod install
```

That's it — no config plugin, no `postinstall`, no manual linking. The C++ core
is compiled into your app by the New Architecture build (CocoaPods on iOS,
CMake / `externalNativeBuild` on Android).

**Expo:** `npx expo install react-native-figma-shadow` then `npx expo prebuild`
(or use EAS Build). It won't run in Expo Go.

**Yarn 3+ (Berry):** ships a modern `exports` map and installs cleanly. Use the
`node-modules` linker — Metro, CocoaPods and Gradle autolinking all need a real
`node_modules` tree, so Yarn PnP is not supported by the RN toolchain:

```yaml
# .yarnrc.yml
nodeLinker: node-modules
```

## Quick start

```tsx
import { View, StyleSheet } from 'react-native';
import { Shadow } from 'react-native-figma-shadow';

function Card() {
  return (
    <Shadow shadow="0 4px 20px rgba(0,0,0,0.12)" borderRadius={16}>
      <View style={styles.card}>{/* ... */}</View>
    </Shadow>
  );
}

const styles = StyleSheet.create({
  card: { width: 320, height: 180, borderRadius: 16, backgroundColor: '#fff' },
});
```

`<Shadow>` wraps its child and paints the shadow behind it. Layout stays neutral
— the shadow bleeds into space the component reserves with negative margins, so
siblings are positioned as if only the child were there.

**Give the shadow the same `borderRadius` as your child** so the shape lines up.

## Recipes

```tsx
// Multiple layers — comma-separated, straight from Figma / CSS
<Shadow shadow="0 2px 4px rgba(0,0,0,.08), 0 12px 32px rgba(0,0,0,.12)" borderRadius={16}>
  <Card />
</Shadow>

// Inner shadow (Figma "Inner shadow"). Pass the fill to <Shadow>, not the child,
// so the shadow sits above it like CSS.
<Shadow shadow="inset 0 2px 8px rgba(0,0,0,0.25)" borderRadius={12} backgroundColor="#fff">
  <TextInput style={{ height: 44 }} />
</Shadow>

// Per-corner radius (e.g. a bottom sheet)
<Shadow shadow="0 -8px 24px rgba(0,0,0,.14)" borderTopLeftRadius={20} borderTopRightRadius={20}>
  <BottomSheetBody />
</Shadow>

// Press-to-lift
function LiftButton() {
  const [down, setDown] = useState(false);
  return (
    <Pressable onPressIn={() => setDown(true)} onPressOut={() => setDown(false)}>
      <Shadow
        shadow={down ? '0 2px 6px rgba(0,0,0,.2)' : '0 10px 24px rgba(0,0,0,.2)'}
        borderRadius={12}
      >
        <View style={styles.button} />
      </Shadow>
    </Pressable>
  );
}
```

More in [docs/RECIPES.md](docs/RECIPES.md).

## API

### `<Shadow>`

| Prop | Type | Notes |
| --- | --- | --- |
| `shadow` | `string` | A CSS `box-shadow` value. A leading `box-shadow:` and a trailing `;` are tolerated. Empty / `none` renders nothing. |
| `borderRadius` | `number` | Uniform corner radius of the shadow shape. Match your child. |
| `borderTopLeftRadius`<br>`borderTopRightRadius`<br>`borderBottomRightRadius`<br>`borderBottomLeftRadius` | `number` | Per-corner override. |
| `backgroundColor` | `string` | Fill painted inside the content box — **above** drop shadows, **below** inset shadows and below the children (CSS background paint order). Use this instead of a background on the child when you have an inset shadow. |
| `style` | `ViewStyle` | Applied to the outer layout box. |
| …`ViewProps` | | `testID`, `onLayout`, `pointerEvents`, accessibility props, etc. pass through to the underlying view. |

### Helpers

```ts
import { parseBoxShadow, computeBleed } from 'react-native-figma-shadow';

parseBoxShadow('0 4px 20px rgba(0,0,0,.15)');
// [{ offsetX: 0, offsetY: 4, blur: 20, spread: 0, inset: false }]

computeBleed(parseBoxShadow('0 4px 20px rgba(0,0,0,.15)'));
// { left: 40, top: 40, right: 40, bottom: 44 }   (how far the shadow reaches past the box)
```

## Why the numbers finally match

"Blur" means different things across systems:

| System | `blur` value is… |
| --- | --- |
| Figma / CSS `box-shadow` | ≈ 2× the Gaussian σ |
| iOS `shadowRadius` | ≈ the Gaussian σ |
| Android `elevation` | not configurable |

This library normalizes everything to the CSS / Figma definition (`σ = blur / 2`),
so the number you paste is the number you get — on both platforms.

## How it works

1. `<Shadow>` parses the value on the JS side only to compute the **bleed** — how
   far the shadow extends past the box — and reserves that space with negative
   margins so layout stays neutral.
2. The native Fabric view hands the raw string, content size, corner radii, fill
   and pixel ratio to the shared C++ `render()`.
3. C++ parses the declaration, rasterizes every layer (plus the optional fill)
   into one premultiplied RGBA8888 bitmap in CSS paint order, and memoizes it.
4. iOS sets the bitmap as a `CALayer`'s `contents`; Android blits it in
   `dispatchDraw` behind the children. Neither platform does any blur math.

Full details in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## How it compares

| | **figma-shadow** | core RN `boxShadow` | [shadow-2] | [Skia] `<Shadow>` | [fast-shadow] | [drop-shadow] |
|---|:---:|:---:|:---:|:---:|:---:|:---:|
| Identical on iOS **and** Android | ✅ one rasterizer | ⚠️ different blur per platform | ⚠️ SVG, close-ish | ✅ one engine | ⚠️ tuned, not exact | ❌ iOS≠Android |
| Paste a CSS / Figma string | ✅ | ✅ | ❌ props | ❌ props | ❌ props | ❌ props |
| True Gaussian blur | ✅ | ✅ | ❌ gradient approx | ✅ | ✅ | ✅ |
| Multiple · `inset` · spread | ✅·✅·✅ | ✅·✅·✅ | ❌·❌·✅ | ✅·✅·⚠️ | ❌·❌·❌ | ❌·❌·❌ |
| Peer dependencies | **none** | none | `react-native-svg` | `@shopify/react-native-skia` | none | none |
| Added download / arch | **~50–100 KB** | 0 | ~0.5–1 MB | **~4–8 MB** | ~10–20 KB | ~10–20 KB |
| List caching | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Decorates a normal view | ✅ | ✅ | ✅ | ❌ (`<Canvas>`) | ✅ | ✅ |
| Old architecture | ❌ | ❌ | ✅ | ✅ | interop | ✅ |

[shadow-2]: https://www.npmjs.com/package/react-native-shadow-2
[Skia]: https://shopify.github.io/react-native-skia/
[fast-shadow]: https://www.npmjs.com/package/react-native-fast-shadow
[drop-shadow]: https://www.npmjs.com/package/react-native-drop-shadow

Reach for this when a design has to match on both platforms, or for long lists of
identically-shadowed cards. Reach for **core `boxShadow`** if "roughly right per
platform" is fine, or **Skia** if you already ship it. Full breakdown in
[docs/COMPARISON.md](docs/COMPARISON.md).

## Troubleshooting

**`Unimplemented component: <FigmaShadowView>`** — the native component isn't
registered. Do a clean iOS reinstall so codegen regenerates:

```sh
rm -rf ios/Pods ios/Podfile.lock ios/build "$HOME/Library/Developer/Xcode/DerivedData"/*
cd ios && pod install
```

Also confirm your app is on the **New Architecture** (it is by default on RN
0.76+).

**The shadow is clipped** — an ancestor has `overflow: 'hidden'`. That clips
`box-shadow` in CSS too. Remove it, or move the clip.

**The shadow appears a frame after the content** — expected. The first raster of
a given shadow runs on a background thread; after that it's cached and instant.
Pass a stable `shadow` string (don't rebuild it inline every render) so the cache
hits.

**Android build: `Cannot get property 'kotlinVersion'`** — fixed in 0.1.0.
Upgrade.

## Limitations

- **Shape shadows only** — follows `borderRadius`, not the alpha of arbitrary
  content (text, transparent PNGs).
- **New Architecture only** (RN 0.76+).
- No animated / interpolatable shadow props yet — snap between two `<Shadow>`
  states, or cross-fade.
- Not yet exercised across a wide device matrix.

## Roadmap

Toward 1.0: device-matrix testing, perf profiling, edge-case hardening, API
freeze. After 1.0: `drop-shadow` alpha-follow, animated props, a design-token
layer, `react-native-web`. Full list and rationale in
[docs/ROADMAP.md](docs/ROADMAP.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). The shared C++ core has a standalone test
harness (`npm run cpp-test`) that needs no device.

## License

MIT © [RaoMK](https://github.com/RaoMK)
