
# foo_vis_spectrum_analyzer

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that implements a spectrum analyzer panel.

It is an attempt to recreate the [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) component by fismineur 
and the [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW) for foobar2000 64-bit.

![Screenshot](Resources/Bars.png?raw=true "Screenshot")

![Screenshot](Resources/Curve.png?raw=true "Screenshot")

## Features

* Fast Fourier (FFT) and Constant-Q (CQT) transforms
* Multiple frequency range and smoothing options
* Color schemes
* Uses DirectX rendering.
* Supports the Default User Interface (DUI) and the [Columns User Interface](https://yuo.be/columns-ui) (CUI).
* Supports foobar2000 2.0 and later (32-bit and 64-bit version)

## Requirements

* [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
* Tested on Microsoft Windows 10 and later.
* Tested with [Columns UI](https://yuo.be/columns-ui) 2.1.0.

## Getting started

* Double-click `foo_vis_spectrum_analyzer.fbk2-component`.

or

* Import `foo_vis_spectrum_analyzer.fbk2-component` into foobar2000 using the "*File / Preferences / Components / Install...*" menu item.

## Developing

### Requirements

To build the code you need:

* [Microsoft Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/) or later
* [foobar2000 SDK](https://www.foobar2000.org/SDK) 2023-09-23
* [Windows Template Library (WTL)](https://github.com/Win32-WTL/WTL) 10.0.10320
* [Columns UI SDK](https://yuo.be/columns-ui-sdk) 7.0.0

The following library is included in the code:

* [Project Nayuki FFT](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)

To create the deployment package you need:

* [PowerShell 7.2](https://github.com/PowerShell/PowerShell) or later

### Setup

Create the following directory structure:

    3rdParty
        columns_ui-sdk
        WTL10_10320
    bin
        x86
    foo_vis_spectrum_analyzer
    out
    sdk

* `3rdParty/columns_ui-sdk` contains the Columns UI SDK 7.0.0.
* `3rdParty/WTL10_10320` contains WTL 10.0.10320.
* `bin` contains a portable version of foobar2000 64-bit for debugging purposes.
* `bin/x86` contains a portable version of foobar2000 32-bit for debugging purposes.
* `foo_vis_spectrum_analyzer` contains the [Git](https://github.com/stuerp/foo_vis_spectrum_analyzer) repository.
* `out` receives a deployable version of the component.
* `sdk` contains the foobar2000 SDK.

### Building

Open `foo_vis_spectrum_analyzer.sln` with Visual Studio and build the solution.

### Packaging

To create the component first build the x86 configuration and next the x64 configuration.

## Change Log

v0.7.0.0-beta-5, 2024-xx-xx

* New: Styles. Moved all the fragmented visual parameters (color, opacity thickness) into one unified style system.
    * Upon first use the old settings are converted to styles and sensible defaults are set for the new features.
    * The "Draw Band Background" option has been removed. It has been replaced by the color source of the dark and light band color.
    * The "Artwork & Dominant Color" background mode has been removed. It has been replaced by the Dominant Color source.
* New: The background and curve peak line and area can be styled.
* New: X-axis mode 'Octaves' shows only 'C' notes; Mode 'Notes' shows all notes.
* Improved: Minor performance optimizations due to styles.
* Improved: Optimized the calculation of the Curve visual.
* Fixed (Beta 4 regression): Removed the DirectComposition code that enabled transparent child windows. It was fun while it lasted.

v0.7.0.0-beta-4, 2024-01-29

* New: File path of the artwork can be specified as a file path or a title formatting script that returns a file path.
* Fixed (Beta 3 regression): Crash when resizing the component.

v0.7.0.0-beta-3, 2024-01-28

* New: Brown-Puckette constant Q transform.
* New: Background mode "None" does not draw a background resulting in transparent element.
* New: Full Screen mode for Columns UI.
* Improved: Bar width and peak indicator thickness get rounded to pixels for a more visually pleasing result.

v0.7.0.0-beta-2, 2024-01-26

* New: Acoustic weighting filters (A-, B-, C-, D- and M-weighting (ITU-R 468))
* Improved: Curve mode no longer has vertical lines at the beginning and the end of the spectrum.
* Improved: The Curve line color can be specified.
* Improved: The Curve Peak line color can be specified.
* Fixed (Beta 1 regression): Crash when switching to full screen or when modifying the layout.

v0.7.0.0-beta-1, 2024-01-21

* New: The cover art of the playing track can be used as background image.
  * Opacity can be specified.
  * The dominant color of the cover art can be used as a filler.
* New: The dominant colors of the cover art can be used to generate a gradient.
  * Color scheme "Artwork" turns on the feature.
  * Between 2 and 256 colors can be selected.
  * Lightness threshold determines which light colors will be ignored.
* New: The Curve visual has a peak line.
* New: Dark mode support.
* New: Fading AIMP peak mode, a combination of AIMP and Fade Out mode.
* Improved: Curve mode can use a horizontal gradient.
* Improved: The curve is no longer visible as a flat line when no track is playing.

v0.6.0.3, 2024-01-13

* Improved: The sample window size for the FFT is now calculated based on the sample rate and the FFT size.
* Fixed: The spectrum was not cleared after stopping a paused track.
* Fixed: Wrong note names in the tooltip when using a non-default number of bands per octave.

v0.6.0.2, 2024-01-02

* Fixed: Finally found a (the?) correct way to use the sample window duration of foobar2000.
* Fixed: Frequency range specification when using notes.
* Fixed: Code to determine Tooltip text.
* Fixed: The Add button of the configuration dialog still displayed the old color dialog.

v0.6.0.1, 2024-01-01

* Fixed: Bug fix for the broken improved DPI check.

v0.6.0.0, 2023-12-31

* New: Curve visualization with customizable line width and area opacity.
* New: The position of the custom gradient colors can be specified individually.
* New: The color dialog can create colors with a specific opacity.
* Improved: Added fall-back case for pre-Windows 10 1903 versions. Completely untested. Your mileage may vary.
* Improved: Hardened the reading and writing of the configuration.
* Improved: Various minor tweaks with focus on stability and future expandability:
  * The graph is drawn completely even when there is no track playing.
  * The peak indicators animation continues when the track stops playing or when it is paused.
  * The configuration dialog page is remembered.
* Improved: The x axis shows labels for the complete specified frequency range.
* Fixed: Crash when adding multiple instances to CUI (Columns User Interface) layouts.
* Fixed: Removed background 'flash' when the element gets created.
* Fixed: The spin controls of the Pitch setting used a wrong divider.
* Fixed: Rewrote the handling of the spin controls to fix range checking and acceleration handling.

v0.5.1.0, 2023-12-14

* New: Support for DUI and CUI default user interface colors.
* New: LED mode.
* New: The color scheme can be used as a horizontal gradient.
* New: The color for the background of white and black 'keys' can be selected separately.
* New: ([Galss](https://hydrogenaud.io/index.php/topic,125031.msg1036200.html#msg1036200)) window function.
* Improved: Rendering occurs on a separate thread resulting in smoother animation.
* Improved: Re-designed configuration dialog to fit on installations scaled higher than 100%.
* Fixed: Some settings were reset when the configuration dialog re-opened during the same session.
* Fixed: Scaling method was available when other than Linear distribution was selected.
* Fixed: Bug in Median, RMS and RMS Sum summation method.

v0.5.0.0, 2023-12-08

* New: Columns UI support.
* Fixed: Tooltips display at the correct position on installations scaled higher than 100%.

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

## Acknowledgements / Credits

* Peter Pawlowski for the [foobar2000](https://www.foobar2000.org/) audio player. ![foobar2000](https://www.foobar2000.org/button-small.png)
* [TF3RDL](https://codepen.io/TF3RDL/pens/) for his [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW), the advice and the help.
* [Project Nayuki](https://www.nayuki.io/page/free-small-fft-in-multiple-languages)
* [Holger Stenger](https://github.com/stengerh) for [foo_vis_oscilloscope](https://github.com/stengerh/foo_vis_oscilloscope_d2d).
* fismineur for [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) that inspired this component.
* Oleg V. Polikarpotchkin and Peter Lee for their [Bezier Spline](https://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit) article.
* [Bedapisl](https://github.com/bedapisl) for [Fast ColorThief](https://github.com/bedapisl/fast-colorthief).

## Reference Material

* [The Audio Frequency Spectrum Explained](https://www.headphonesty.com/2020/02/audio-frequency-spectrum-explained)
* [Fast Fourier Transformation FFT - Basics](https://www.nti-audio.com/en/support/know-how/fast-fourier-transform-fft)
* [Sine Tone Generator](https://www.audiocheck.net/audiofrequencysignalgenerator_sinetone.php)
* [A440](https://en.wikipedia.org/wiki/A440_(pitch_standard))
* [Constant-Q transform](https://en.wikipedia.org/wiki/Constant-Q_transform)
* [Mel-frequency cepstrum](https://en.wikipedia.org/wiki/Mel-frequency_cepstrum)
* [Mel scale](https://en.wikipedia.org/wiki/Mel_scale)

## Test Material

* [The SOS Audio Test Files](https://www.soundonsound.com/techniques/sos-audio-test-files)

## Links

* Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)
* Repository: [https://github.com/stuerp/foo_vis_spectrum_analyzer.git](https://github.com/stuerp/foo_vis_spectrum_analyzer.git)
* Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
