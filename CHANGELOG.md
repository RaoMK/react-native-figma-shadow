# Changelog

## 0.1.0-alpha.9

- **Validated the rasterizer against a true Gaussian.** The fast SDF/error-
  function path is within ~5-8% of an ideal CSS `box-shadow` at the peak and
  near-exact in the body; a `testAccuracyVsGaussian` case guards it. RN's own
  Android `boxShadow` is the one that over-spreads.
- **Removed the `highQuality` prop.** Its quadrature path was ~20x slower and
  produced an identical result.
- Per-corner radii now apply to the `backgroundColor` fill too (previously only
  the top-left radius).

## 0.1.0-alpha.8

- Reserve 4 sigma of bleed instead of 3, so large / offset / spread shadows fade
  completely instead of ending in a faint hard edge at the raster buffer bound.

## 0.1.0-alpha.7

- **iOS: fix `Unimplemented component: <FigmaShadowView>`** (the real cause this
  time). React Native's generated `RCTThirdPartyFabricComponentsProvider`
  forward-declares `FigmaShadowViewCls()` with **C linkage**; our `.mm` defined
  it as a plain C++ function, so the symbol the provider looked up never
  resolved and the component fell back to "Unimplemented". The definition is now
  `extern "C"`, the `.mm` imports `RCTFabricComponentsPlugins.h`, and
  `codegenConfig` gains the `ios.components` / `type: "all"` entries that a
  `create-react-native-library` Fabric view ships with.
- Android: `FigmaShadowViewPackage` reports an empty module-info map (view
  managers register through `createViewManagers`, not `ReactModuleInfo`).
- podspec: use `min_ios_version_supported` instead of a hard-coded floor.

## 0.1.0-alpha.6

- **Fix inset shadows on Android** (and simplify both platforms). The optional
  `backgroundColor` fill is now composited into the shadow bitmap by the shared
  C++ core, in the correct CSS paint order (drop shadows, then fill, then inset
  shadows). iOS and Android just blit one bitmap — no more separate `CALayer` /
  `Canvas.drawPath` fill pass that was dropping the inset darkening on Android.
- Android: JNI copies the bitmap row-by-row when the Android bitmap stride is
  padded, instead of a flat `memcpy`.

## 0.1.0-alpha.5

- **iOS: fix `Unimplemented component: <FigmaShadowView>`** at runtime. The
  library shipped a `react-native.config.js` that declared `dependency.platforms`
  explicitly; the empty `ios: {}` entry stopped autolinking from registering the
  Fabric component. Removed the file entirely — `codegenConfig` in `package.json`
  plus standard autolinking discovers the component on both platforms.

  After upgrading, do a clean iOS reinstall:
  `rm -rf ios/Pods ios/Podfile.lock ios/build ~/Library/Developer/Xcode/DerivedData/* && (cd ios && pod install)`

## 0.1.0-alpha.4

- **Android: fix `Cannot get property 'kotlinVersion' on extra properties
  extension`** at evaluation time. The library no longer declares its own
  `buildscript` block — the Android Gradle, Kotlin and React Native Gradle
  plugins are inherited from the host app's root classpath, which removes the
  cross-version conflicts entirely.
- Android: use `compileSdk` / `minSdk` / `targetSdk` (the non-deprecated form).

## 0.1.0-alpha.3

- **iOS: fix build errors** `Redefinition of 'SharedColor'` and `No member named
  'parseColor' in namespace 'figmashadow'`. Our C++ headers (`Color.h`, `Types.h`,
  …) share basenames with React Native's and were being flattened onto the shared
  header search path. They are now kept out of `source_files` (`.cpp` only) and
  reached exclusively as `figmashadow/<name>.h`.

## 0.1.0-alpha.2

- **Android: fix `FigmaShadowViewManager cannot be cast to IViewGroupManager`**
  crash on mount. The component has children, so the view manager must extend
  `ViewGroupManager`, not `SimpleViewManager`.
- Android: read `compileSdk`/`targetSdk`/`minSdk` from Gradle properties again,
  bump defaults to 36/36/24, take Kotlin/AGP versions from the host app where
  available (works with React Native 0.76 through 0.87+).

## 0.1.0-alpha.1

- First publish. Shared C++ rasterizer (parser, CSS color parser, analytic
  Gaussian rounded-rectangle blur), Fabric component for iOS and Android, JS
  `<Shadow>` wrapper. Not yet run through a full native app build.
