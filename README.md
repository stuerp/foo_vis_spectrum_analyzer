
# foo_vis_spectrum_analyzer

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that implements a spectrum analyzer panel.

It is an attempt to recreate the [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) component by fismineur 
and the [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW) for foobar2000 64-bit.

![Screenshot](assets/Bars.png?raw=true "Screenshot")

<sup>Spectrum analyzer Bars Mode with gradient colors</sup>

![Screenshot](assets/Curve.png?raw=true "Screenshot")

<sup>Spectrum analyzer Curve Mode with album art background, a peak meter and a level meter</sup>

![Screenshot](assets/MultipleGraphs.png?raw=true "Screenshot")

<sup>Multiple Spectrum analyzer graphs for the separate channels</sup>

![Screenshot](assets/Spectogram.png?raw=true "Screenshot")

<sup>Horizontal scrolling spectogram visualization</sup>

![Screenshot](assets/Radial-Bars.png?raw=true "Screenshot")

<sup>Spectrum analyzer Radial Bars Mode</sup>

![Screenshot](assets/Radial-Curve.png?raw=true "Screenshot")

<sup>Spectrum analyzer Radial Curve Mode</sup>

![Screenshot](assets/Oscilloscope.png?raw=true "Screenshot")

<sup>Oscilloscope with Y-axis in dBFS</sup>

![Screenshot](assets/Oscilloscope-XY.png?raw=true "Screenshot")

<sup>Oscilloscope in X-Y mode</sup>

## Features

- Spectrum Analyzer (Bars, Curve, Radial Bars), Spectogram, Peak Meter, Level Meter and Oscilloscope visualizations.
- Fast Fourier (FFT), Constant-Q (CQT), Sliding Windowed Infinite Fourier (SWIFT) and Analog-style transforms
- Multiple frequency range and smoothing options
- Multiple graphs
- Styling of all visual elements
- Artwork background and color extraction
- Uses DirectX rendering.
- Supports the Default User Interface (DUI) and the [Columns User Interface](https://yuo.be/columns-ui) (CUI).
- Supports dark mode.
- Supports foobar2000 2.0 and later (32-bit and 64-bit version).

## Requirements

- Tested on [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
- Tested on Microsoft Windows 10 and later.
- Tested with [Columns UI](https://yuo.be/columns-ui) 2.1.0.

## Getting started

- Double-click `foo_vis_spectrum_analyzer.fbk2-component`.

or

- Import `foo_vis_spectrum_analyzer.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.

## Usage

You can find the user guide [here](docs/README.md).

## Developing

### Requirements

To build the code you need:

- [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
- [foobar2000 SDK](https://www.foobar2000.org/SDK) 2025-03-07
- [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320
- [Columns UI SDK](https://yuo.be/columns-ui-sdk) 7.0.0

The following library is included in the code:

- [Project Nayuki FFT](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)

To create the deployment package you need:

- [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later

### Setup

Create the following directory structure:

    3rdParty
        columns_ui-sdk
        WTL10_10320
    bin
    bin.x86
    foo_vis_spectrum_analyzer
    out
    sdk

- `3rdParty/columns_ui-sdk` contains the Columns UI SDK 7.0.0.
- `3rdParty/WTL10_10320` contains WTL 10.0.10320.
- `bin` contains a portable version of foobar2000 64-bit for debugging purposes.
- `bin.x86` contains a portable version of foobar2000 32-bit for debugging purposes.
- `foo_vis_spectrum_analyzer` contains the [Git](https://github.com/stuerp/foo_vis_spectrum_analyzer) repository.
- `out` receives a deployable version of the component.
- `sdk` contains the foobar2000 SDK.

### Building

Open `foo_vis_spectrum_analyzer.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v0.9.0.0-beta2, 2025-11-xx

- 
 
You can read the full history [here](docs/History.md).

## Acknowledgements / Credits

- Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
- [TF3RDL](https://codepen.io/TF3RDL/pens/) for the advice, the help and his:
  - [Frequency bands spectrum analyzer using either FFT or CQT](https://codepen.io/TF3RDL/pen/poQJwRW)
  - [SWIFT, Sliding Windowed Infinite Fourier Transform](https://codepen.io/TF3RDL/pen/JjBzjeY)
  - [Analog-style spectrum analyzer and sliding DFT visualization using AudioWorklet](https://codepen.io/TF3RDL/pen/MWLzPoO)
- [Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)
- [Holger Stenger](https://github.com/stengerh) for [foo_vis_oscilloscope](https://github.com/stengerh/foo_vis_oscilloscope_d2d).
- fismineur for [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) that inspired this component.
- Oleg V. Polikarpotchkin and Peter Lee for their [Bezier Spline](https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit) article.
- [Bedapisl](https://github.com/bedapisl) for [Fast ColorThief](https://github.com/bedapisl/fast-colorthief).

## Reference Material

- [The Audio Frequency Spectrum Explained](https://www.headphonesty.com/2020/02/audio-frequency-spectrum-explained)
- [Fast Fourier Transformation FFT - Basics](https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft)
- [Sine Tone Generator](https://www.audiocheck.net/audiofrequencysignalgenerator_sinetone.php)
- [A440](https://en.wikipedia.org/wiki/A440_(pitch_standard))
- [Constant-Q transform](https://en.wikipedia.org/wiki/Constant-Q_transform)
- [Mel-frequency cepstrum](https://en.wikipedia.org/wiki/Mel-frequency_cepstrum)
- [Mel scale](https://en.wikipedia.org/wiki/Mel_scale)
- [Sliding windowed infinite Fourier transform](https://en.wikipedia.org/wiki/Sliding_DFT)

## Test Material

- [The SOS Audio Test Files](https://www.soundonsound.com/techniques/sos-audio-test-files)
- [HTML5 AAC Audio Playback Tests - Multichannel](https://www2.iis.fraunhofer.de/AAC/multichannel.html)

## Links

- Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)
- Repository: [https://github.com/stuerp/foo_vis_spectrum_analyzer.git](https://github.com/stuerp/foo_vis_spectrum_analyzer.git)
- Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)
- Wiki: [https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Spectrum_Analyzer_(foo_vis_spectrum_analyzer)](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Spectrum_Analyzer_(foo_vis_spectrum_analyzer)).
## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
