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

## Build (macOS — VST3 + AU + Standalone)

Requires Xcode (or Command Line Tools) and CMake ≥ 3.22. The `FORMATS` line in
CMakeLists adds AU automatically on Darwin. The UI uses the system WKWebView —
no extra dependencies.

```sh
# clone with submodules (JUCE)
git clone --recurse-submodules <repo-url> && cd opna-vst

# universal binary (Apple Silicon + Intel)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --parallel
```

Outputs: `build/Nineight_artefacts/Release/{VST3,AU,Standalone}`; JUCE copies
them to `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`.
The AU only shows up in Logic/GarageBand after a plugin rescan; validate with:

```sh
auval -v aumu Opna Nin8
```

Note: unsigned builds may need Gatekeeper quarantine removed on other machines
(`xattr -dr com.apple.quarantine <plugin>`), or proper codesign/notarization
for distribution.

## Build (Windows — VST3 + Standalone)

Requires Visual Studio 2022 (Desktop C++ workload), CMake, and git (the
configure step applies the JUCE webview patch with `git apply`).

```bat
git clone --recurse-submodules <repo-url>
cd opna-vst
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Outputs: `build/Nineight_artefacts/Release/{VST3,Standalone}`; the VST3 is
copied to `%COMMONPROGRAMFILES%\VST3` (may need an elevated shell, or set
`COPY_PLUGIN_AFTER_BUILD FALSE` and copy manually).

The UI needs the **WebView2 Evergreen Runtime** — preinstalled on Windows 11
and most updated Windows 10 machines; otherwise install it from the
[WebView2 download page](https://developer.microsoft.com/microsoft-edge/webview2/).
Without it the editor comes up blank (audio still works).

Note: the Linux-only pieces (webview patch behavior, `HostEnvSanitizer`) are
no-ops on macOS/Windows; the same `ui/webui.zip` is embedded on all platforms.

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

| JS → C++ | Purpose |
| --- | --- |
| `synthReady()` (native fn, round-trip) | returns `{sampleRate, nativeRate, patch}` on UI boot |
| `uiEvent` (one-way `emitEvent`) | all hot-path traffic: `{cmd: noteOn/noteOff/allNotesOff/setParam/setOpEnvelope/post, …}` — no response roundtrip, so knob drags can't flood the message loop |

Events (C++ → JS, 30 Hz while editor visible): `paramChanged`, `noteOn`/`noteOff`
(host MIDI → key highlight), `waveform` (2048-byte AnalyserNode-format scope data).

## Status / TODO

- [x] Native engine, MIDI, params, state, WebView UI (verified in Standalone on Linux)
- [ ] Phase 4: SSG scene / rhythm pads / ADPCM-B / MML passthrough (`synthPost` cases)
- [ ] Phase 5: CI matrix (Win VST3, macOS VST3+AU universal), pluginval runs
- [ ] Scene adopts DAW-restored patch on editor open (currently boots with default preset)
- [ ] Mono-mode portamento glide for host MIDI

Licenses: ymfm BSD-3 (Aaron Giles), JUCE 8 (GPLv3/commercial), UI assets from the opna repo.
