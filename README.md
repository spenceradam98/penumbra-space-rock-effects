# Penumbra (working name)

A space-rock studio effects rack plugin (VST3 + standalone): fuzz/overdrive, EQ, a VCS3-style
ring-mod and filter stage, phaser/flanger/ADT/chorus, rotary speaker, multi-head tape delay
with reverse, varispeed glide and freeze, plate/spring/hall reverb, and auto-pan/swirl. 25
presets for guitars and synths, 5 UI looks. See `docs/DESIGN.md`.

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release            # fetches JUCE 8.0.4
# or: cmake -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release
ctest --test-dir build                                # engine tests
```

macOS builds target macOS 11+ universal (Intel + Apple Silicon). No AVX2 or other `-march`
flags are used; the DSP is scalar C++. Windows builds use the static runtime.

Tools (optional): `-DPENUMBRA_BUILD_SNAPSHOTS=ON` builds a UI screenshot tool, `-DPENUMBRA_BUILD_TOOLS=ON` builds the preset demo renderer used for `docs/preset-demo.mp3` and preset level matching.

Layout: `Source/engine` is plain C++ (all DSP modules, parameter table, presets) with tests in
`tests/`. `Source/plugin` is the JUCE wrapper and UI.
