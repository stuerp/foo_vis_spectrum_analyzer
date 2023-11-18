
# foo_vis_spectrum_analyzer

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that implements a spectrum analyzer panel.

It is an attempt to recreate the [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) component by fismineur for foobar2000 64-bit.

## Features

* Uses Direct2D rendering.
* Supports foobar2000 2.0 (32-bit and 64-bit version)
* Supports dark mode.

## Requirements

* Tested on Microsoft Windows 10 and later.
* [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)

## Getting started

* Double-click `foo_vis_spectrum_analyzer.fbk2-component`.

or

* Import `foo_vis_spectrum_analyzer.fbk2-component` into foobar2000 using "File / Preferences / Components / Install...".

## Developing

The code builds out-of-the box with Visual Studio.

### Requirements

To build the code:

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later.
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2023-05-10
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320.

The following library is included:

* [Project Nayuki FFT](https://www.nayuki.io/page/free-small-fft-in-multiple-languages))

To create the deployment package:

* [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later.

### Setup

Create the following directory structure:

    3rdParty
        WTL10_10320
    bin
        x86
    foo_vis_spectrum_analyzer
    out
    sdk

* `3rdParty/WTL10_10320` contains WTL 10.0.10320.
* `bin` contains a portable version of foobar2000 2.0 for debugging purposes.
* `bin/x86` contains a portable version of foobar2000 1.6 for debugging purposes.
* `foo_vis_spectrum_analyzer` contains the [Git](https://github.com/stuerp/foo_vis_spectrum_analyzer) repository.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open `foo_vis_spectrum_analyzer.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v0.1.0.5, 2023-xx-xx

* New: Implemented first version of the configuration dialog.
  * It's not much to look at, has a lot of rough edges and not user-friendly but it works. Use it to play with but except things to change.
  * The JSON file will be ignored from now on, possible later to be re-used for loading and saving presets.
* New: Implemented logarithmic Y axis.
* New: Experimental "Fade-Out" mode for the peak indicators.

v0.1.0.4, 2023-11-16

* New: Implemented color schemes.
* New: Implemented the peak value indicators.
* Improved: Replaced the FFT with one that supports complex values and non-radix-2 FFT sizes.
* Improved: Improved and refactored rendering code.

v0.1.0.3, 2023-11-14

* New: Implemented the frequency bands of [AveePlayer](https://aveeplayer.com/).
* New: Implemented the smoothing methods of the spectrum.
* New: Added some DirectX debug logging to the console. Set the "LogLevel" parameter to 1 to enable; Use 6 (default) to disable.

v0.1.0.2, 2023-11-13

* New: The configuration is now read from a JSON file. Note: Not all parameters are implemented yet.
* New: Implemented all frequency scaling methods.
* New: Implemented all coefficient summation methods.
* New: The X axis has 3 extra display modes.

v0.1.0.1, 2023-11-12

* Fixed: Spectrum calculation used the wrong FFT size. A 440Hz A4 note now appears in the right place.
* Fixed: Crash in 32-bit version.

v0.1.0.0, 2023-11-12, *"Scratchin' the itch"*

* Initial release.

## To Do

* Support ColumnsUI.

## Acknowledgements / Credits

* Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)
* [Holger Stenger](https://github.com/stengerh) for [foo_vis_oscilloscope](https://github.com/stengerh/foo_vis_oscilloscope_d2d).
* fismineur for [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)).
* [filoe](https://github.com/filoe) for [cscore](https://github.com/filoe/cscore). I took a long look at the FFT analyzer.
* [TF3RDL](https://hydrogenaud.io/index.php?action=profile;u=160476) for his [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW), the advice and the help.

## Reference Material

* [The Audio Frequency Spectrum Explained](https://www.headphonesty.com/2020/02/audio-frequency-spectrum-explained)
* [Fast Fourier Transformation FFT - Basics](https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft)
* [Sine Tone Generator](https://www.audiocheck.net/audiofrequencysignalgenerator_sinetone.php)
* [A440](https://en.wikipedia.org/wiki/A440_(pitch_standard))

## Links

* Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)
* Repository: [https://github.com/stuerp/foo_vis_spectrum_analyzer.git](https://github.com/stuerp/foo_vis_spectrum_analyzer.git)
* Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
