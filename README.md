
# foo_vis_spectrum_analyzer

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that implements a spectrum analyzer panel.

It is an attempt to recreate the [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) component by fismineur 
and the [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW) for foobar2000 64-bit.

![Screenshot](/Resources/Screenshot.png?raw=true "Screenshot")

## Features

* Fast Fourier (FFT) and Constant-Q (CQT) transforms
* Multiple frequency range and smoothing options
* Color schemes
* Uses Direct2D rendering.
* Supports foobar2000 2.0 and later (32-bit and 64-bit version)

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
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2023-09-23
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320.

The following library is included:

* [Project Nayuki FFT](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)

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
* `bin` contains a portable version of foobar2000 x64 for debugging purposes.
* `bin/x86` contains a portable version of foobar2000 x86 for debugging purposes.
* `foo_vis_spectrum_analyzer` contains the [Git](https://github.com/stuerp/foo_vis_spectrum_analyzer) repository.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open `foo_vis_spectrum_analyzer.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v0.4.2.1, 2023-12-05

* Improved: Some internal tweaks and fixes.
* Fixed: The default size of the configuration dialog was not initialized.

v0.4.2.0, 2023-12-03

* New: Hovering over the spectrum displays a tooltip with the center frequency and the note name (when applicable) of the band over which the mouse hovers.
* New: You can select which channels will be used during the transform in the configuration dialog.
* New: You can select the window function that will be applied to the samples.
* Improved: Made color scheme editing more resilient.
* Improved: Added 30fps refresh rate.
* Improved: The configuration dialog does no longer restrict the minimum size. You can resize it to any size no in case some controls are not accessible.

v0.4.1.1, 2023-11-29

* New: Amplitude increment/decrement is configurable.
* Improved: Some numeric controls are editable again. Range checking has been improved.
* Improved: The high amplitude label and grid line on the Y axis are now fully visible.
* Improved: Labels don't overlap anymore.
* Fixed: The "Band Background" label was cut off.
* Fixed: Range checking on some configuration panel controls.
* Fixed: Content scaling should be DPI-aware now.
* Fixed regression: Resetting the configuration failed.

v0.4.0.0, 2023-11-26

* New: Most colors and the custom color scheme can be modified in the configuration dialog.
* Improved: Each instance of the component now has its own configuration. Note: The fullscreen version is a separate instance.
* Improved: Added spin buttons to most numeric controls.
* Changed: Mel scale mapping is now called Triangular Filter Bank scale mapping.

v0.3.0.0, 2023-11-20

* New: Implemented a first version of a Constant-Q transform (CQT). Default is still FFT.
* New: A custom FFT size can be specified in the configuration dialog.
* New: A sample rate-based FFT size can be specified in the configuration dialog. The FFT size will be calculated based on the duration specified and the sample rate of the track being played.
* New: Implemented Mel scale mapping of the FFT coefficients.
* New: X-axis and Y-axis can hidden by selecting the "None" mode of each of the axes.
* Improved: Added a versioning mechanism to the settings of the component. Unfortunate side-effect: Any configuration will be reset.
* Improved: Tweaked the layout and the size of the configuration dialog a little bit.
* Improved: Removed the client edge of the window for a cleaner look.
* Fixed: The configuration dialog disabled the decibel settings when selecting a logarithmic Y-axis scale.
* Fixed: Opening the visualization when a track was already playing did not show the spectrum.

v0.2.0.0, 2023-11-18

* New: Implemented a configuration dialog.
  * It's not pretty to look at, has a lot of rough edges and is not user-friendly but it works. Use it to play with but except things to change (and hopefully improve).
  * The JSON file will be ignored from now on, possibly later to be re-used for loading and saving presets.
* New: Implemented logarithmic Y-axis.
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
* New: The X-axis has 3 extra display modes.

v0.1.0.1, 2023-11-12

* Fixed: Spectrum calculation used the wrong FFT size. A 440Hz A4 note now appears in the right place.
* Fixed: Crash in 32-bit version.

v0.1.0.0, 2023-11-12, *"Scratchin' the itch"*

* Initial release.

## To Do

* Support ColumnsUI.

## Acknowledgements / Credits

* Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [TF3RDL](https://codepen.io/TF3RDL/pens/) for his [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW), the advice and the help
* [Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)
* [Holger Stenger](https://github.com/stengerh) for [foo_vis_oscilloscope](https://github.com/stengerh/foo_vis_oscilloscope_d2d).
* fismineur for [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) that inspired this component

## Reference Material

* [The Audio Frequency Spectrum Explained](https://www.headphonesty.com/2020/02/audio-frequency-spectrum-explained)
* [Fast Fourier Transformation FFT - Basics](https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft)
* [Sine Tone Generator](https://www.audiocheck.net/audiofrequencysignalgenerator_sinetone.php)
* [A440](https://en.wikipedia.org/wiki/A440_(pitch_standard))
* [Constant-Q transform](https://en.wikipedia.org/wiki/Constant-Q_transform)
* [Mel-frequency cepstrum](https://en.wikipedia.org/wiki/Mel-frequency_cepstrum)
* [Mel scale](https://en.wikipedia.org/wiki/Mel_scale)

## Links

* Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)
* Repository: [https://github.com/stuerp/foo_vis_spectrum_analyzer.git](https://github.com/stuerp/foo_vis_spectrum_analyzer.git)
* Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
