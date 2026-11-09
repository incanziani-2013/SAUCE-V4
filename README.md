# SAUCE V1 PRO — prod.shiny

A JUCE/CMake Windows x64 VST3 source project for an 808 saturation / clipping effect.

## Included
- JUCE audio processor with parameter smoothing.
- 4x oversampled nonlinear saturation and asymmetric clipping.
- Input/output trim, wet/dry, auto-gain trim, sub-preservation blend and transient envelope control.
- Eight factory program settings.
- Host state serialization and an A/B parameter snapshot toggle.
- Input/output peak meters and a custom-drawn purple cup illustration.
- CMake build configuration and Windows build script.

## Build requirements
- Windows 10/11 x64
- Visual Studio 2022 with **Desktop development with C++**
- CMake 3.22+
- Git
- Internet connection for first configure (JUCE 8.0.6 is fetched by CMake)

Run `BUILD_SAUCE.bat` from this folder. The first build may take several minutes.

## Install
After a successful build, find `SAUCE.vst3` under `build/SAUCE_artefacts/Release/VST3/`
(or the generated CMake artefacts folder) and copy the bundle to:
`C:\Program Files\Common Files\VST3\`

Then rescan plugins in FL Studio.

## Engineering status / honest limitations
This package contains source code, not a precompiled VST3. Compilation and FL Studio
loading have not been verified in this environment. The oversampling DSP path is fixed at
4x; the oversampling selector is present as a host parameter but does not yet switch the
engine rate. Sub preservation is a clean-branch blend, not a frequency-split crossover.
The cup is a lightweight vector illustration rather than a physically simulated liquid.
Custom preset file import/export and full commercial QA remain outside this build.
