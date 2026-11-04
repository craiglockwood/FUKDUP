# FUKDUP

Glitch destruction unit. VST3 (Windows + macOS) and AU (macOS), built with JUCE 8.

## Modules

| Module    | Controls              | What it does |
|-----------|-----------------------|--------------|
| STUTTER   | Div, Repeats          | Grabs a tempo-synced slice (1/2 to 1/64) and loops it. A fresh slice is grabbed every *Div x Repeats*. |
| REVERSE   | Size                  | Plays the incoming audio backwards in grains of 40 ms to 1 s. |
| TAPE STOP | Time                  | Slows the signal to a halt (pitch drops with speed) over 50 ms to 3 s. Release to snap back. |
| CRUSH     | Bits, Rate Div        | Bit-depth reduction (16 down to 1 bit) plus sample-rate reduction (hold every 1 to 64 samples). |
| GATE      | Div, Density, Smooth  | Random tempo-synced chopping. Density is the chance each step stays open. Smooth rounds off the edges. |

**Master:** Mix (dry/wet), Feedback (feeds the glitched output back into the stutter/reverse capture buffer so repeats build on themselves), Output (-24 to +12 dB).

Signal order: Stutter or Reverse (stutter wins if both are on) -> Tape Stop -> Crush -> Gate -> Mix -> Output.

The effects sync to your DAW's tempo and playhead. With the transport stopped they run on an internal clock at the host tempo (120 BPM if none is given).

Every on/off switch is a normal automatable parameter, so you can draw glitches in with automation.

## Building

You need CMake 3.22+ and a C++17 compiler (Xcode on macOS, Visual Studio 2022 on Windows). JUCE is downloaded automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Universal Mac build (Apple Silicon + Intel):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

Plugins appear in `build/FUKDUP_artefacts/Release/`.

### Installing

- **macOS:** builds are copied automatically to `~/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/Components`. For AU, you may need to restart Logic or run `killall -9 AudioComponentRegistrar`.
- **Windows:** copy `FUKDUP.vst3` into `C:\Program Files\Common Files\VST3`.

### No compiler? Use GitHub

Push this folder to a GitHub repo. The included workflow (`.github/workflows/build.yml`) builds Mac and Windows versions and attaches them under the run's **Artifacts**. Unsigned Mac downloads need: `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/FUKDUP.component` (same for the .vst3).

## Notes

- A standalone app is also built, handy for quick testing.
- Plugin codes: manufacturer `Fkdp`, plugin `Fkd1`. Change these in `CMakeLists.txt` before distributing if you want your own identity.
- JUCE is free for personal use and under a revenue threshold; check the JUCE licence before selling.
