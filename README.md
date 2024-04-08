
# foo_vis_spectrum_analyzer

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that implements a spectrum analyzer panel.

It is an attempt to recreate the [foo_musical_spectrum](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Components/Musical_Spectrum_(foo_musical_spectrum)) component by fismineur 
and the [Audio Spectrum project](https://codepen.io/TF3RDL/pen/poQJwRW) for foobar2000 64-bit.

![Screenshot](Resources/Bars.png?raw=true "Screenshot")

![Screenshot](Resources/Curve.png?raw=true "Screenshot")

![Screenshot](Resources/MultipleGraphs.png?raw=true "Screenshot")

## Features

* Fast Fourier (FFT), Constant-Q (CQT), Sliding Windowed Infinite Fourier (SWIFT) and Analog-style transforms
* Multiple frequency range and smoothing options
* Multiple graphs
* Styling of all visual elements
* Artwork background and color extraction
* Uses DirectX rendering.
* Supports the Default User Interface (DUI) and the [Columns User Interface](https://yuo.be/columns-ui) (CUI).
* Supports dark mode.
* Supports foobar2000 2.0 and later (32-bit and 64-bit version).

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

v0.7.5.2, 2024-04-xx

* Improved: Peak Meter
  * Calibrated the peak meter according to the IEC 61606:1997 / AES17-1998 standard (RMS +3).
  * Added style to display the RMS value as text.
  * Reduced jitter.
* Improved: Increased the maximum amplitude to +6dB.
* Improved: Optimized rendering the axes a little bit while preserving the more imported labels:
  * The first and last label are always rendered.
  * The C notes are always rendered the Notes mode of the X-axis.
* Fixed: Setting the LED size and LED gap both to zero caused the component to freeze.

v0.7.5.1, 2024-04-06

* Fixed: Peak Meter axes now behave as intended.
* Fixed: Sample rate initialization and Nyquist frequency marker display when the component is used in a popup window.
* Fixed: Disappearing artwork when the component is used in a popup window.

v0.7.5.0, 2024-04-04

* New: Spectogram visualization.
  * Static or scrolling
* New: Peak Meter visualization.
* New: Built-in SoX color scheme and corresponding non-linear amplitude color map suited for the spectogram visualization.
* New: The context menu can be used to load a preset.
* New: Artwork Fit mode. Defaults to "Fit Big".
* Improved: Double-clicking a preset activates it.
* Improved: Edge cases for the scaling and position of the spectrum bars and curve vs. the X-axis.
* Improved: LED Mode is also applied to the bar background.
* Improved: The LED size and gap can be configured.
* Improved: The font of the X and Y-axis can be configured in all visualizations.
* Fixed: Privacy issue: The preset path will no longer be saved to a preset file.
* Fixed: The spin buttons were not updated when switching between multiple graphs in the configuration dialog.
* Fixed: Inconsistent state of the color buttons and values when switching between styles in the configuration dialog.
* Fixed: User interface colors were not activated after loading a preset.
* Fixed: The custom Solid color was not always saved correctly.
* Fixed: Deadlock condition specific to CUI when playback was set to resume at startup.
* Fixed: Memory leak in tool tips when resizing the component.
* Fixed: Artwork should now consistently be rendered when requested.

v0.7.4.1, 2024-03-14

* Fixed: CUI configurations with styles that use color source "User Interface" caused a hard crash.

v0.7.4.0, 2024-03-13

* New: Amplitude-based color selection.
  * The colors of the spectrum bars can be set based on the amplitude of the frequency when using a horizontal gradient.
  * The colors of the current gradient are used to create a color list. Works with fixed, custom and artwork-based gradients.
  * Only the Bar Area and Bar Peak Area styles support this feature.
* Improved: Gradient editing
  * Adding a color to a gradient will no longer recalculate the position of all colors. The added color will get a position between its predecessor and successor.
  * Removing a color from a gradient will leave all remaining positions untouched.
* Fixed: The user interface colors and Windows colors were not consistently used and updated after the introduction of styles.
* Fixed: Several minor fixes and tweaks to clean up inconsistencies and rough edges.

v0.7.3.0, 2024-03-09

* New: Presets.
  * The configuration of the component can be written to and read from preset files (*.fvsa).
  * The default location of the preset files is the root of your profile directory but this can be changed.
* New: Extra styles.
  * Bar Area: The new name for the Bar Spectrum style.
  * Bar Top: Defines the style for the top of the spectrum bar. Defaults to color source None for backwards compatibility.
  * Peak Area: Defines the style for the area below the peak indicator. Defaults to color source None for backwards compatibility.
  * Peak Top: The new name for the Peak Indicator style.
* Improved: LED mode properly renders the 'LEDs' as blocks.
* Improved: The font of the graph description can be specified.
* Improved: Dark mode tooltips.
* Fixed: Multiple controls were not updated when switching graphs in the configuration dialog.

v0.7.2.0, 2024-03-03

* Changed: The grid lines can be displayed without an X or Y-axis.
  * X-axis and Y-axis mode "None" still disables everything.
  * The X-axis or Y-axis labels can be controlled with the top, bottom, left and right settings.
  * The X-axis Line and Y-axis Line style have been renamed to Vertical and Horizontal Grid Line.

v0.7.2.0-alpha-1, 2024-03-02

* New: Analog-style spectrum analysis. Based on https://codepen.io/TF3RDL/pen/MWLzPoO.
* Improved: Performance optimization. Rendering will stop when the main window is minimized.
* Improved: Small performance optimization in SWIFT.
* Fixed: The style list box in the configuration dialog was not cleared before it was filled again after a reset.
* Fixed (Regression): Gradient brushes are created again with the colors in the same order as shown in the configuration dialog.

v0.7.1.0, 2024-02-29

* Fixed: Reversing the colors of a gradient did not recalculate the position of the colors. Instead it spread out the color evenly.
* Changed: To avoid confusion color source None for the background has been disabled until it can be properly implemented.

v0.7.1.0-rc-3, 2024-02-28

* Improved: Tweaked the background painting again to prevent flashes when resizing the component.
* Fixed: The selected gradient color scheme was not activated after startup.

v0.7.1.0-rc-2, 2024-02-27

* Fixed (Regression): Non-radix-2 FFT's caused a crash.

v0.7.1.0-rc-1, 2024-02-26

* New: The style of the graph description background can be specified.
* New: A marker for the Nyquist frequency can be enabled on the Styles page.
* Improved: The frequency tooltip tries to stay within borders of the component.
* Fixed: The graph description will not be shown when it is left empty or when its color source is set to "None".
* Fixed: The gradient brushes were not recreated when the graph was resized.
* Fixed: X-axis scaling did not obey the selected scaling function.
* Fixed: Most visual elements with color source "None" ignored the setting.

v0.7.1.0-beta-2, 2024-02-22

* New: Multiple graphs.
  * Each graph can be configured to show the analysis of one or more channels.
  * A graph can be flipped horizontally or vertically.
  * Graphs can be stacked vertically or on a horizontal line.
* New: Context menu item to freeze and unfreeze the component.
* Improved: Rendering and spectrum computation are not performed when the component window is hidden (to reduce the load on the computer).

v0.7.1.0-beta-1, 2024-02-14

* New: Sliding Windowed Infinite Fourier Transform (SWIFT). Based on https://codepen.io/TF3RDL/pen/JjBzjeY.
* New: Reaction Alignment parameter allow you to control the delay between the actual playback and the visualization.
* New: The X-axis can be displayed on top, below or on both sides of the spectrum.
* New: The Y-axis can be displayed on left, right or on both sides of the spectrum.
* New: Option to enable or disable the rendering of the mirror image of the spectrum (Anything above the Nyquist frequency). Default is disabled.
* Changed: Y-axis mode Logarithmic is now called Linear/n-th root.
* Fixed: Added some missing range checks in the configuration dialog that could lead to a crash.
* Fixed (Regression): Incorrect background color during startup caused a bright flash in dark mode.

v0.7.0.0, 2024-02-12

* New: Tooltips that provides some explanation about the various configuration settings.

v0.7.0.0-rc-2, 2024-02-10

* Fixed (RC 1 regression): A couple of small bugs that could cause horrible crashes.

v0.7.0.0-rc-1, 2024-02-07

* New: Styles. Moved all the fragmented visual parameters (color, opacity thickness) into one unified style system.
    * Upon first use the old settings are converted to styles and sensible defaults are set for the new features.
    * The "Draw Band Background" option has been removed. It has been replaced by the color source of the dark and light band color.
    * The "Artwork & Dominant Color" background mode has been removed. It has been replaced by the Dominant Color source.
* New: The background and curve peak line and area can be styled.
* Changed: X-axis mode 'Octaves' shows only 'C' notes; Mode 'Notes' shows all notes.
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
* Improved: The Curve peak line color can be specified.
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
* [TF3RDL](https://codepen.io/TF3RDL/pens/) for the advice, the help and his:
  * [Frequency bands spectrum analyzer using either FFT or CQT](https://codepen.io/TF3RDL/pen/poQJwRW)
  * [SWIFT, Sliding Windowed Infinite Fourier Transform](https://codepen.io/TF3RDL/pen/JjBzjeY)
  * [Analog-style spectrum analyzer and sliding DFT visualization using AudioWorklet](https://codepen.io/TF3RDL/pen/MWLzPoO)
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
* [Sliding windowed infinite Fourier transform](https://en.wikipedia.org/wiki/Sliding_DFT)

## Test Material

* [The SOS Audio Test Files](https://www.soundonsound.com/techniques/sos-audio-test-files)
* [HTML5 AAC Audio Playback Tests - Multichannel](https://www2.iis.fraunhofer.de/AAC/multichannel.html)

## Links

* Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)
* Repository: [https://github.com/stuerp/foo_vis_spectrum_analyzer.git](https://github.com/stuerp/foo_vis_spectrum_analyzer.git)
* Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)

## License

![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)
