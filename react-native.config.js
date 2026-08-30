/**
 * The shadow raster lives in a standalone `.so` that this package builds itself
 * (see android/build.gradle + android/CMakeLists.txt) and loads with
 * `System.loadLibrary`, so we deliberately do NOT set `cmakeListsPath` here —
 * that would make the host app try to build the same sources a second time.
 *
 * The Fabric component descriptor is generated from `codegenConfig` in
 * package.json by the host app's build.
 */
module.exports = {
  dependency: {
    platforms: {
      android: {
        componentDescriptors: ['FigmaShadowViewComponentDescriptor'],
      },
      ios: {},
    },
  },
};
