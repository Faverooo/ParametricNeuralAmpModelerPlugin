# ParametricNAM VST3

Play a `.pnam` guitar amp model in REAPER or another DAW with VST3 support. Each model brings its own amplifier name and adjustable knobs.

## Install and play

1. Download the installer for your system from [Releases](https://github.com/Faverooo/ParametricNeuralAmpModelerPlugin/releases/latest). Close your DAW and run the Windows setup or macOS package. On Linux, run the downloaded `.run` file with `bash`.
2. Reopen your DAW, rescan your VST3 plug-ins if needed, and add **ParametricNAM** to a track with a clean guitar input.
3. Click **Load model** and open a `.pnam` file. Set your DAW to the model’s sample rate, typically **48 kHz**.
4. Use **Output** for manual volume. For **Level Match**, play a short riff or loop while it measures. Click **Relearn** to measure again.

Models are separate downloads. You can make your own with [Capture](https://github.com/Faverooo/ParametricNAM-Capture) and [Training](https://github.com/Faverooo/ParametricNAM-Training). Keep a model at its original path so your DAW project can reopen it.

For GitHub builds, set the `PNAM_CORE_SHA` Actions variable to the matching [Core](https://github.com/Faverooo/ParametricNeuralAmpModelerCore) commit. The workflow contains the build steps.

## License

The project code is [MIT licensed](LICENSE). [JUCE](https://juce.com/legal/) has its own license terms for builds and redistribution.
