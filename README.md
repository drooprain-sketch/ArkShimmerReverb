# Ark Shimmer Reverb

**Ark Shimmer Reverb** is an original cinematic shimmer reverb plugin prototype designed for worship, ambient, live performance, guitars, vocals, pads, piano, and stage transitions.

This repository contains the full C++/JUCE source project for the first functional version. It is not a clone of any existing plugin; it uses an original FDN reverb tank plus a pitch-feedback shimmer engine.

## Current status

Version: `0.1.0 prototype`

Implemented:

- VST3/Standalone JUCE project.
- Stereo FDN reverb tank with modulated delay lines.
- Pitch-feedback shimmer engine.
- Pitch modes: `+12`, `+7`, `+12 +7`, `-12`, `Dual Detune`, `Choir`.
- Modes: `Worship`, `Cinematic`, `Live`, `Dark Ark`, `Infinite`.
- Mix, Decay, Size, Shimmer, Tone, Mod, Width, Duck.
- Low Cut, High Cut, Pre Delay, Input, Output.
- Freeze and Clear Tail controls.
- Custom GUI with Ark visual identity.
- Initial factory preset sheet.

## Important note

This package is **source code**, not a compiled `.vst3` binary. To produce the plugin file, compile it with JUCE and CMake on your target machine.

## Build requirements

- CMake 3.22 or newer.
- C++17 compiler.
- JUCE 7 or JUCE 8.
- A VST3-compatible host or DAW for testing.

Recommended on Windows:

- Visual Studio 2022 with Desktop development with C++ workload.
- JUCE downloaded locally.
- CMake installed and available in PATH.

## Build instructions — Windows

### Option A: with local JUCE path

```powershell
cd ArkShimmerReverb
cmake -B build -DJUCE_PATH="C:/JUCE"
cmake --build build --config Release
```

The VST3 will usually be created inside:

```text
build/ArkShimmerReverb_artefacts/Release/VST3/Ark Shimmer Reverb.vst3
```

### Option B: automatic JUCE fetch

```powershell
cd ArkShimmerReverb
cmake -B build
cmake --build build --config Release
```

This requires internet access because CMake will fetch JUCE from GitHub.

## Build instructions — macOS

```bash
cd ArkShimmerReverb
cmake -B build -DJUCE_PATH=/path/to/JUCE
cmake --build build --config Release
```

The generated formats are currently:

- VST3
- Standalone app

AU can be added in `CMakeLists.txt` by changing:

```cmake
FORMATS VST3 Standalone
```

to:

```cmake
FORMATS VST3 AU Standalone
```

## Suggested first test

Use it as a send/aux reverb:

- Mix: 100%
- Mode: Live
- Pitch: +12 +7
- Quality: Live
- Decay: 5–7 s
- Size: 65–80%
- Shimmer: 25–40%
- Low Cut: 180–220 Hz
- High Cut: 8–12 kHz
- Duck: 35–50%

## Main DSP concept

```text
Input
  ↓
Pre Delay
  ↓
Low/High Cut
  ↓
Early Diffusion
  ↓
FDN Reverb Tank
  ↓
Pitch Shimmer Engine
  ↓
Pitch-feedback injection
  ↓
Ducking / Width / Safety Limit
  ↓
Dry-Wet Mix
  ↓
Output
```

## Development notes

This v0.1 prototype prioritizes a complete, readable DSP architecture over final commercial polish. Before commercial use, the following should be tested and refined:

- Latency and CPU on target machines.
- A/B tests at 44.1, 48, 88.2, and 96 kHz.
- Stereo mono-compatibility.
- Parameter automation smoothing.
- Long-tail stability at extreme Decay/Shimmer values.
- Fine-tuning of pitch shifter grain artifacts.
- Signed installers for Windows/macOS.
- Preset browser and preset serialization.

## File structure

```text
ArkShimmerReverb/
├── CMakeLists.txt
├── README.md
├── Presets/
│   └── FactoryPresets.md
└── Source/
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp
    ├── PluginEditor.h
    ├── PluginEditor.cpp
    └── DSP/
        └── ArkDSP.h
```

## Product identity

Name: Ark Shimmer Reverb  
Subtitle: Celestial Reverb & Shimmer Engine  
Core sound: cinematic, wide, warm, luminous, worship-ready.  
Initial target format: VST3 64-bit.
