# Changelog

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
