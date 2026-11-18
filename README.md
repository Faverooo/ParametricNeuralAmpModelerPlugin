# ParametricNAM VST3

A mono or stereo guitar-amp effect that loads PNAM v1 models. Each stereo channel runs its own instance of the mono model. The eight host automation slots keep stable IDs and follow the control order in the loaded model. The editor shows the model's amplifier name, control labels and dial values.

## Build

Place a checkout of ParametricNeuralAmpModelerCore with PNAM support alongside this repository and initialize its Eigen submodule.

```sh
git -C ../ParametricNeuralAmpModelerCore submodule update --init Dependencies/eigen
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ParametricNAM_VST3
```

CMake uses an installed JUCE 8 package when available and otherwise fetches JUCE 8.0.6. Pass `-DPNAM_JUCE_DIR=/path/to/JUCE` for an offline JUCE checkout. `PNAM_CORE_DIR` and `PNAM_EIGEN_DIR` override the Core and Eigen locations.

## Test in REAPER

1. Add the generated `ParametricNAM.vst3` bundle from `build/ParametricNAM_artefacts/Release/VST3/` to your REAPER VST search path and rescan.
2. Set the audio device to the model's sample rate. For `NightTrain-030.pnam`, use **48 kHz**.
3. Add ParametricNAM to a track with a dry guitar recording. Open the model in the plugin and move the controls or automate the active control slots.
4. Save and reopen the REAPER project to check restoration. Plugin state saves the model file path and control values, so keep the model at that path.

The first version passes dry audio through when no model is loaded or when the host sample rate differs from the model. It accepts mono PNAM models with one to eight controls.

## Level Match

Enable **Level Match** and play a representative loop with the controls held steady. The plugin measures five seconds of active input and compares the dry and model-output RMS. It then applies a fixed output trim limited to ±18 dB. Blocks below −60 dBFS RMS on the input do not count toward the measurement.

Turn Level Match off for an uncompensated comparison. Use **Relearn** after a substantial control change. Opening a new model clears the previous match. The completed trim and on/off state are saved with the project.

## GitHub Actions

Set the repository Actions variable `PNAM_CORE_SHA` to the full 40-character commit SHA from `Faverooo/ParametricNeuralAmpModelerCore`. Build artifacts are available on each successful workflow run. A `v*` tag publishes the platform ZIP files after the build matrix passes. Each archive records its Core SHA in `CORE_REVISION.txt`.

Extract the archive and copy the entire `ParametricNAM.vst3` bundle to your VST3 folder.

- Linux: `~/.vst3/`
- Windows: `C:\Program Files\Common Files\VST3\`
- macOS: `~/Library/Audio/Plug-Ins/VST3/`
