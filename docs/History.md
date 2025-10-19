
# foo_vis_spectrum_analyzer History

v0.9.0.0-alpha3, 2025-10-17

- New: Oscilloscope time-domain mode
  - Supports X-axis with time labels.
  - Signal gain.
  - Optional phosphor decay effect with Gauss sigma and color decay factor setting.
- New: Oscilloscope X-Y mode.
  - Supports X and Y-axis.
  - X and Y signal gain.
  - Optional phosphor decay effect with Gauss sigma and color decay factor setting.
- Improved: Rendering stops when the host window gets minimized instead of the main window to improve behavior in foo_flowin.
- Improved: Various little useability tweaks and fixes in the Configuration dialog.
- Changed: Upgraded rendering code from render targets to device contexts.
- Fixed: Delay between playback and visualization. The backbuffer was not initialized.
 
v0.9.0.0-alpha2, 2025-10-09

- New: Separate styles for X-axis vs. horizontal grid lines and Y-axis vs. vertical grid lines.
- New: LED Mode has an *Integral Size* setting that will render the LEDs at full instead of fractional block sizes.
- New: The configuration dialog will only show the styles that are used by the current visualization.
- Improved: Oscilloscope visualization.
  - X and Y-axis support
  - Y-axis none, dBFS and linear dBFS modes.
  - Only the selected channels are rendered.
- Fixed: Rendering bug in horizontally scrolling spectogram.

v0.9.0.0-alpha1, 2025-10-05

- New: Radial Curve visualization.
- New: Oscilloscope visualization.
- Fixed: Too few samples were taken into account during analysis (frame count vs. sample count). Thanks to TF3RDL for detecting and reporting it.

v0.8.1.0, 2025-09-26

* New: The graph horizontal alignment setting has a new *Fit* option to make the Bar visualization use the complete width of the graph (minus the X-axis when enabled).
* New: If the artwork can't be retrieved or is not available the stub image will be used.
* Improved: The tooltip will only show the note name if the note's frequency falls within the frequency range of the spectrum bar.
* Improved: Going forward older versions of the component will be able to read the configuration and preset files from newer versions in case you want or need to downgrade.
* Fixed: Artwork not showing up when a track is already playing when switching to fullscreen. (0.8.0.0-beta2 regression)
* Fixed: Restored compatibility with foobar 1.6.18.
* Fixed: Spectrum bar top and peak size miscalculation. (0.7.6.2 regression)

v0.8.0.0, 2025-09-22

* New: The album art type that is shown on the background can be selected in the configuration dialog.
* New: The log verbosity can be set on the **Common** page of the configuration dialog.
* Improved: Stability of album art rendering with multiple instances and heavy load.
* Fixed: Rendering negative infinity in the peak meter readout was broken. (Beta 2 regression)
* Fixed: Analyzer initialization after channel configuration changes.

v0.8.0.0-beta3, 2025-09-19

* New: [User guide](docs/README.md) documenting all the configuration settings.
* New: When the `Tool tips` option is disabled you can still get a tool tip when you click and hold the left mousebutton over a frequency band.
* Improved: Radial Bars
  * Implemented horizontal and amplitude-based gradient, peak mode.
* Fixed: Weird behavior in IIR filter bank/analog-style analyzer. (Provided by TF3RDL)
* Fixed: Stability of the tooltip.
* Fixed: SWIFT analyzer (Regression)
* Fixed: Artwork color sorting (Regression)

v0.8.0.0-beta2, 2024-08-18

* New: Radial Bar visualization.
* Improved: Smoothing factor can be specified with 2 decimals. (Forum request)
* Improved: The spectrum bars can be horizontally aligned in the graph area. (Forum request)
* Fixed: Rounding errors when calculating the gauge metrics with non-default DPI settings.
* Fixed: Gauge scale lines were too short (Regression).
* Fixed: Slow spectrum rendering with DSD streams.
* Builds with foobar2000 SDK 2024-08-07.

v0.8.0.0-beta1, 2024-05-01

* New: Left/Right and Mid/Side level meter.
  * The left/right channel pair is selectable.
* Spectogram
  * New: Vertical scrolling and static spectogram. A special setting is available to align the spectogram with a spectrum bars visualization over or under the spectogram.
  * Improved: Overall polishing and removal of glitches.
* Added: Separate peak and RMS level read outs to the peak meter.
* Fixed: An old color bug in the owner-drawn menu list.

v0.7.6.2, 2024-04-17

* Peak Meter
  * Changed: Removed the 1/sqrt(2) correction from the RMS reading after a long discussion on the forum.

v0.7.6.1, 2024-04-17

* Peak Meter
  * Fixed: The RMS text reading did not appear if the corresponding X-axis was disabled.
  * Fixed: Work-around for disappearing LED bars (Odd behaviour of FillOpacityMask())
 
v0.7.6.0, 2024-04-16

* Peak Meter
  * Added: Option to allow the user to get readings compliant with IEC 61606:1997 / AES17-1998 standard (RMS +3).
  * Added: The gap between the gauges can be configured.
  * Changed: Removed the 3.01 dB from the peak value.
  * Changed: Tweaked the coordinate calculations a bit to produce a more polished result.
* Improved: The context menu will put a checkmark next to the last selected preset.

v0.7.5.4, 2024-04-13

* Peak Meter
  * Changed: Swiched the unit from dB to dBFS.
  * Fixed: Some channel configurations (e.g. with side channels) were not displayed correctly.

v0.7.5.3, 2024-04-13

* Improved: Peak Meter
  * Added style to display the peak and RMS values larger than 0dB.
  * Added style to display the top peak value.
  * RMS Window is now configurable. Defaults to 300ms. This makes the RMS value more stable. The RMS is relative to 0 dBFS + 3.01 dB.
  * The level values were not calculated correctly when the selected channels did not correspond to the channel configuration of the track.
* Fixed: Overlap of the X-axis labels (Regression)

v0.7.5.2, 2024-04-10

* Improved: Peak Meter
  * Calibrated the peak meter according to the IEC 61606:1997 / AES17-1998 standard (RMS +3).
  * Added style to display the RMS value as text.
  * Reduced jitter.
* Improved: Increased the maximum amplitude to +6dB.
* Improved: Optimized rendering the axes a little bit and tried to preserve the more imported labels.
* Improved: Artwork fit mode can use the full component window instead of just the client area of the graph.
* Fixed: Setting the LED size and LED gap both to zero caused the component to freeze.
* Fixed: Artwork fit mode "Fill" was implemented backwards.

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
