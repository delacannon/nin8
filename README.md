# NINEIGHT — OPNA (YM2608) FM Synth Plugin

C++ port of the [opna](../opna) web app "NINEIGHT" as **VST3 / AU / Standalone** (JUCE 8).

- **Audio**: native `ymfm::ym2608` — the *same* emulator the web app compiles to WASM. Register packing, voice allocation, resampler and rhythm-ROM init are line-for-line ports of `@opna/core`, so the plugin sounds identical.
- **UI**: the *actual* Phaser 3 web UI (PC-98 CRT aesthetic) embedded via JUCE 8 `WebBrowserComponent`, talking to the native engine over a JS↔C++ bridge (`window.__JUCE__`).
- 52 host-automatable parameters (algorithm, feedback, LFO, gain, pan, poly, 4 ops × 11); state = APVTS XML.

## Build (Linux)

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang-12 -DCMAKE_CXX_COMPILER=clang++-12   # gcc 9 breaks on JUCE 8 harfbuzz
cmake --build build --parallel
```

Outputs land in `build/Nineight_artefacts/Release/{VST3,Standalone}` and are copied to `~/.vst3`.

Deps: `libwebkit2gtk-4.0-dev` + usual JUCE X11/ALSA packages.

## Refresh the embedded UI

Requires the opna monorepo checked out as a sibling (`../opna`) with the JUCE-mode
patches (JuceBackend.ts, AudioEngine juce branches, `vite/config.juce.mjs`).

```sh
scripts/build-ui.sh        # vite build → ui/webui.zip (vendored, committed)
node scripts/gen-rhythm-rom.mjs   # regenerate assets/ym2608_rhythm_rom.bin
```

## Offline audio check

```sh
clang++-12 -O2 -std=c++17 -I third_party/ymfm/src -o build/render_test \
  tools/render_test.cpp third_party/ymfm/src/ymfm_{opn,adpcm,ssg}.cpp
build/render_test assets/ym2608_rhythm_rom.bin /tmp/c4.wav   # renders piano C4, fails if silent
```

## Bridge protocol

| Native fn (JS → C++) | Purpose |
|---|---|
| `synthReady()` | returns `{sampleRate, nativeRate, patch}` on UI boot |
| `synthNoteOn/Off(note)` | `"C4"` → native voice allocator |
| `synthSetParam(id, value)` | APVTS `setValueNotifyingHost` (DAW automation records) |
| `synthPost(msg)` | unchanged AudioWorklet message shapes (`write`, `reset`, …) |

Events (C++ → JS, 30 Hz while editor visible): `paramChanged`, `noteOn`/`noteOff`
(host MIDI → key highlight), `waveform` (2048-byte AnalyserNode-format scope data).

## Status / TODO

- [x] Native engine, MIDI, params, state, WebView UI (verified in Standalone on Linux)
- [ ] Phase 4: SSG scene / rhythm pads / ADPCM-B / MML passthrough (`synthPost` cases)
- [ ] Phase 5: CI matrix (Win VST3, macOS VST3+AU universal), pluginval runs
- [ ] Scene adopts DAW-restored patch on editor open (currently boots with default preset)
- [ ] Mono-mode portamento glide for host MIDI

Licenses: ymfm BSD-3 (Aaron Giles), JUCE 8 (GPLv3/commercial), UI assets from the opna repo.
