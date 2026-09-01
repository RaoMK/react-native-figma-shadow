# Contributing

Thanks for helping out. This package has three layers:

```
cpp/figmashadow/   shared C++ core (parser, color parser, rasterizer, cache)
src/               JS: codegen spec + <Shadow> wrapper + bleed parser
ios/ , android/    thin Fabric glue that just blits the C++ bitmap
```

Almost all real logic is in `cpp/figmashadow/`. The glue layers should stay
boring.

## Setup

```sh
npm install
```

## The fast loop (no device needed)

The C++ core is the interesting part and it has a standalone harness:

```sh
npm run cpp-test
```

This compiles `cpp/test/test_main.cpp` against the core with `clang`/`g++` and
runs it. It covers the color parser, the box-shadow parser, rasterizer
invariants (symmetry, offset direction, inset falloff, premultiplied validity),
determinism, and an **accuracy check against a true separable Gaussian**
(`testAccuracyVsGaussian`) so the fast SDF/erf path can't silently drift.

It also writes a few PPMs to `/tmp/fs_*.ppm` for eyeballing.

JS side:

```sh
npm test          # jest — the bleed parser
npm run typecheck
npm run lint
npm run prepare   # builds lib/ with react-native-builder-bob
```

## The slow loop (device)

`example/App.tsx` is the manual-test screen — see `example/README.md` for how to
run it in a bare RN app. CI also builds a scaffolded app against the packed
tarball on every push (`.github/workflows/ci.yml`).

## Rules of thumb

- **Keep parity.** Anything that affects pixels goes in `cpp/figmashadow/` so
  both platforms get the same bytes. If you find yourself writing platform blur
  code, stop.
- **Don't break the accuracy test.** If `testAccuracyVsGaussian` regresses, the
  shadow no longer matches CSS.
- Match the surrounding style. C++ is Google-ish; Kotlin/TS follow the repo
  lint config.
- Update `CHANGELOG.md` under an `## [Unreleased]` heading (or the next version).

## Releasing

Maintainers only: bump `version` in `package.json`, add the `CHANGELOG.md`
section, push, then run the **Publish to npm** GitHub Action. It publishes to the
`latest` dist-tag and cuts a matching GitHub Release from the changelog.
