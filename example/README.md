# Example

`App.tsx` is a scrollable gallery that renders each shadow twice — once with
`<Shadow>` from this package, once with core RN `style={{ boxShadow }}` — so you
can compare, and check that the left column looks identical on iOS and Android.

This folder is intentionally just the screen. To run it, drop it into a bare
React Native app:

```sh
# 1. a fresh RN app (0.76+, New Architecture on by default)
npx @react-native-community/cli init ShadowExample
cd ShadowExample

# 2. this package
npm install react-native-figma-shadow

# 3. use the example screen
curl -o App.tsx https://raw.githubusercontent.com/RaoMK/react-native-figma-shadow/main/example/App.tsx

# 4. run
cd ios && pod install && cd ..
npm run ios
npm run android
```

CI does essentially this on every push (`.github/workflows/ci.yml` → the
`native-build` job) against the packed tarball, so a broken podspec / Gradle
config fails the build.
