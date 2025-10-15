
# foo_vis_spectrum_analyzer User Guide

Welcome to [foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases). This guide will help you understand how to use the component effectively and get the most out of its features.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Features](#features)
3. [Context Menu](#context-menu)
4. [Configuring](#configuring)
5. [Troubleshooting](#troubleshooting)
6. [FAQs](#faqs)
7. [Reference Material](#reference-material)
8. [History](#history)
9. [Acknowledgements and Credits](#acknowledgements-and-credits)
10. [Support](#support)

---

## Introduction

[foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) is a [foobar2000](https://www.foobar2000.org/) component that that implements a panel that can render different kinds of visualisations of the track being played such as a spectrum analysis, a spectogram and peak meters.

---

### System Requirements

- [foobar2000](https://www.foobar2000.org/download) v2.0 or later (32 or 64-bit). ![foobar2000](https://www.foobar2000.org/button-small.png)
- Hardware supported rendering using Direct2D and DirectWrite
- Tested on Microsoft Windows 10 and later.

### Download foo_vis_spectrum_analyzer

You can download the component from the foobar2000 [Components](https://www.foobar2000.org/components/view/foo_vis_spectrum_analyzer+%28x64%29) repository or from the GitHub [Releases](https://github.com/stuerp/foo_vis_spectrum_analyzer/releases) page.

### Installation

- Double-click `foo_vis_spectrum_analyzer.fbk2-component` or import `foo_vis_spectrum_analyzer.fbk2-component` using the foobar2000 Preferences dialog. Select the **File / Preferences / Components** menu item, select the **Components** page and click the **Install...** button.
- Follow the foobar2000 instructions.

> [!Tip]
> To verify if the installation was successful open the foobar2000 Preferences using the **File / Preferences / Components** menu item and look for **Spectrum Analyzer** in the **Installed components** list.

You can experiment with the component without modifying your layout by selecting **Spectrum Analyzer** from the **View / Visualizations**.

---

## Features

- Fast Fourier (FFT), Constant-Q (CQT), Sliding Windowed Infinite Fourier (SWIFT) and Analog-style transforms
- Multiple frequency range and smoothing options
- Multiple graph types
- Styling of all visual elements
- Artwork background and color extraction
- Uses DirectX rendering.
- Supports the Default User Interface (DUI) and the [Columns User Interface](https://yuo.be/columns-ui) (CUI) both in windowed and full-screen mode.
- Supports dark mode.
- Supports foobar2000 2.0 and later (32-bit and 64-bit version).

---

## Context Menu

Right-click on the component window to display the context menu. The following items are available:

`Configure`

Opens the Configuration dialog. This component has its own configuration dialog instead of a page in the standard foobar2000 preferences dialog.

`Toggle Full-Screen Mode`

Toggles full-screen mode off and on.

> [!Note]
> When using the Default User Interface (DUI) the full-screen version is another instance of the component with its own configuration. When you use the Columns User Interface (CUI) the existing instance will be resized to occupy the full screen.

`Frame Counter`

Enables the display of a rudimentary frame counter.

`Refresh Rate Limit`

Allows you to select the refresh rate of the component animation.

> [!Note]
> Even though the component can redraw its visuals very fast the actual maximum refresh rate is usually determined by the refresh rate of your monitor.

`Presets`

Contains a list of previously saved presets. Select one from the list to activate it. A checkmark will be shown next to the most recent activated preset.

`Freeze`

Prevents the refresh of the component, effectively reducing CPU and GPU use to zero.

---

## Configuring

The configuration dialog is used to change all parameters used by the component. All changes are applied immediately to the component without the need to close it.

A vertical list shows the names of all the configuration **pages**. Click on the name of a page to activate it. Most settings will display a tooltip to explain their purpose.

`Reset`

Resets the configuration to the default values.

`OK`

Closes the dialog box and preserves the changes to the configuration during the current session.

> [!Note]
> The settings will only be saved to your profile when foobar2000 is closed.

`Cancel`

Closes the dialog box and undoes any changes to the configuration.

---

### Transform page

This page contains the controls to configure the Time to Frequency transform that the component will use to visualize the frequencies.

#### Transform group

`Method`

Allows you to select the Time to Frequency domain transform. The following transforms are implemented:

- Fast Fourier (FFT)
- Constant-Q (CQT)
- Sliding Windowed Infinite Fourier (SWIFT)
- Analog-style

`Window function`

Selects the window function that will be applied to the samples (Time domain).

`Window parameter`

Allows you to tweak window functions that support a parameter like Gaussian and Kaiser.

`Window skew`

Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples. Defaults to 0 (None).

`Reaction alignment`

Controls the delay between the actual playback and the visualization.

- Less than zero: All samples are ahead of the playback sample with the first sample equal to the actual playback sample.
- Zero: The first half of samples are behind the current playback sample and the second half are ahead of it.
- Greater than zero: All samples are behind the playback with the last sample equal to the current playback sample.

#### Fourier Transform group

Groups the parameters that influence the Fast Fourier transform.

`No. of bins`

Specifies the number of bins used by the transforms as a number between 64 and 32768.

- Select ''Custom'' to specify a number that is not a power of 2. This will consume a lot more CPU power.
- Select ''Sample rate based'' to specify the size as a duration. F.e. 100ms of a track sampled at 44.1kHz will result in an FFT window of 4410 samples.

`Coefficient aggregation`

Determines which method is used to aggregate the coefficients of the FFT into frequency bins.

- Minimum
- Maximum
- Sum
- Residual Mean Square (RMS)
- RMS Sum
- Average
- Median

`Mapping`

Determines how the FFT coefficients are mapped to the frequency bins.

- Standard
- Triangular Filter Bank
- Brown-Puckette CQT

`Smooth lower frequencies`

When enabled, the band power part only gets used when number of FFT bins to sum for each band is at least two or more.

`Smooth gain transition`

Smooths the frequency slope of the aggregation modes.

`Lanczos kernel`

Determines the size of the Lanczos kernel. The kernel is used to create a smooth transition between the FFT coefficients resulting in a visual pleasing result.

#### Brown-Puckette CQT Mapping group

The settings in this group become available when you select Brown-Puckette CQT for coefficient mapping.

`Bandwidth offset`

Offsets the bandwidth of the Brown-Puckette CQT.

`Bandwidth cap`

Minimum Brown-Puckette CQT kernel size.

`Bandwidth amount`

The Brown-Puckette CQT kernel size.

`Granular bandwidth`

When disabled constrains the bandwidth to powers of 2.

`Kernel shape`

Determines the shape of the Brown-Puckette CQT kernel.

`Kernel shape parameter`

Parameter used by certain window functions like Gaussian and Kaiser windows.

`Kernel asymmetry`

Adjusts how the window function reacts to samples. Positive values makes it skew towards latest samples while negative values skews towards earliest samples.

#### Infinite Impulse Response (IIR) group

The settings in this group become available when you select the SWIFT or the Analog-style transforms.

`Filter bank order`

Determines the order of the filter bank used to calculate the SWIFT and Analog-style transforms.

`Time resolution`

Determines the maximum time resolution used by the SWIFT and Analog-style transforms.

`Bandwidth`

Determines the bandwidth used by the SWIFT and Analog-style transforms.

`Constant-Q`

Uses constant-Q instead of variable-Q in the IIR transforms.

`Compensate bandwidth`

Compensate bandwidth for narrowing on higher order IIR filters banks.

`Use prewarped Q`

Prewarps Q to ensure the actual bandwidth is truly logarithmic at anything closer to the Nyquist frequency.

---

### Frequencies page

This page allows you to specify the frequency range you would like to visualize.

#### Frequencies group

Groups the parameters that determine the frequency range that will be displayed.

`Distribution`

Determines which parameters are used to generate the frequency bands.

- Linear: Generate frequency bands based on the frequency parameters.
- Octaves: Generate frequency bands based on the note parameters.
- AveePlayer: Generate frequency bands used by AveePlayer.

`No. of bands`

Specifies the number of frequency bands to generate (2 to 512) in the specified frequency range.

`Frequency range`

Frequency of the first and last band, 1Hz to 96000Hz. Each frequency is centered in the band.

`Note range`

Available when selecting the ''Octaves'' distribution. Select a range between the C note of octave 0 and B# of octave 11 resulting in maximum 144 notes or 12 octaves.

`Bands per octave`

Number of bands per octave ranging from 1 to 48.

`Pitch`

Frequency of the tuning pitch (A4 = 440.0Hz). Valid values are 1Hz to 96000Hz.

`Transpose`

Determines how many semitones the frequencies will be transposed. Valid values are -24 to 24.

`Scaling`

Determines which algorithm will be used to scale the frequencies:

- Linear
- Logarithmic
- Shifted logarithmic
- Mel (AIMP)
- Bark
- Adjustable Bark
- ERB
- Cams
- Hyperbolic Sine
- n-th Root
- Negative exponential
- Period

`Skew factor`

Affects any adjustable frequency scaling functions like hyperbolic sine and n-th root. Higher values mean a more linear spectrum.

`Bandwidth`

Distance between the low and high frequency boundaries for each frequency band. Valid values are 0.0 to 64.0.

---

### Filters page

#### Acoustic Filters group

This group contains settings to allow you to apply acoustic weighting filters (A-, B-, C-, D- and M-weighting (ITU-R 468)) to the samples.

`Weighting type`

Selects the weighting filter type that will be applied.

`Slope function offset`

Slope function offset expressed in sample rate / FFT size in samples

`Slope`

Frequency slope offset in dB per octave

`Slope offset`

Frequency slope in Hz

`Equalize amount`

Equalization amount

`Equalize offset`

Equalization offset

`Equalize depth`

Equalization depth

`Weighting amount`

Weighting amount

---

### Common page

This page contains settings that the visualizations have in common.

#### Common group

`Smoothing method`

Determines how the spectrum coefficients and the peak meter values are smoothed.

- Average: Calculates a weighted average between the old and the new value of the peak. Uses the smoothing factor as a weight.
- Peak: Retains the maximum between the old and the new value of the peak.

`Smoothing factor`

Specifies the strength of the smoothing (0.0 to 1.0)

`Tooltips`

Enable the check box to see a tooltip with the center frequency and when appropriate, the name of the note, or the frequency band.

`Suppress mirror image`

Prevents the mirror image of the spectrum (anything above the Nyquist frequency) from being rendered.

#### Artwork group

Some visualizations can use artwork to display on the background or to use as a source for a color list. The artwork can come from the playing track or from file location.

> [!Note]
> The color selection runs only when a new track is started.

`No. artwork colors`

The maximum number of colors to select from the artwork.

`Lightness threshold`

Determines when a color is considered light. Expressed as a percentage of whiteness, white being 100%.

`Sort colors by`

Determines how to sort the colors selected from the artwork.

- `None` will start the color list with the color that is used the most in the artwork followed by lesser used colors.
- `Increasing hue` and `Decreasing hue` sort by hue.
- `Increasing lightness` and `Decreasing lightness` sort by lightness.
- `Increasing saturation` and `Decreasing saturation` sort by saturation.

`Show artwork on background`

Displays artwork on the graph background. By default the front cover of the playing track is used. You can override this behavior using the `Artwork file path` setting.

`Artwork type`

Determines which artwork will be displayed on the graph background.

> [!Note]
> The selection only becomes active when a new track is started.

`Fit mode`

Determines how over- and undersized artwork is rendered.

- Free: The artwork will not be scaled.
- Fit big: Artwork that is bigger than the available area will be proportionally scaled.
- Fit width: The width of the artwork is scaled to fit in the available area.
- Fit height: The height of the artwork is scaled to fit in the available area.
- Fill: The width or height of artwork is scaled to fit the available area.

`Fit window`

Enable to use the full component window as available area instead of the client area (the window minus any room taken by axes)

`Artwork opacity`

Determines the opacity of the artwork when displayed.

`Artwork file path`

A fully-qualified file path or a foobar2000 script that returns the file path of an image to display on the graph background.

#### Component

`Log Level`

Allows you to control the messages the component will write to the foobar2000 console.

> [!Warning]
> The **Debug** and **Trace** levels may generate a lot of technical output.

---

### Visualization page

This page contains settings specific to some of the visualizations.

#### Type

Determines the type of visualization of a graph:

##### Bars

The classic frequency spectrum visualization.

![Screenshot](assets/Bars.png?raw=true "Screenshot")

<sup>Spectrum analyzer Bars Mode with gradient colors</sup>

##### Curve

The same as bars but with a smoothed curve instead of bars.

![Screenshot](assets/Curve.png?raw=true "Screenshot")

<sup>Spectrum analyzer Curve Mode with album art background, a peak meter and a level meter</sup>

##### Spectogram

![Screenshot](assets/Spectogram.png?raw=true "Screenshot")

<sup>Horizontal scrolling spectogram visualization</sup>

##### Peak / RMS

Displays the peak and RMS levels of the audio.

##### Balance / Correlation

Displays the balance and mid/side correlation of the audio.

##### Radial Bars

A variation of the Bars visualization that displays the frequency spectrum in a circle.

![Screenshot](assets/Radial-Bars.png?raw=true "Screenshot")

##### Radial Curve

A variation of the Curve visualization that displays the frequency spectrum in a circle.

![Screenshot](assets/Radial-Curve.png?raw=true "Screenshot")

##### Oscilloscope

Shows the shape of the audio signal over time, helping you understand how sound behaves.

**Time-domain mode**

In this mode the X-axis represents the time and the Y-axis represents the amplitude of the signal.

This visualization uses the following settings:

- Graph X-axis and Y-axis settings
  - Select *None* to prevent the axis from being rendered.
- Channel selection
- X and Y signal gain
- Phosphor effect settings
 
![Screenshot](assets/Oscilloscope.png?raw=true "Screenshot")

<sup>Oscilloscope</sup>

![Screenshot](assets/Oscilloscope-dBFS.png?raw=true "Screenshot")

<sup>Oscilloscope with Y-axis in dBFS</sup>

**X-Y mode**

In this mode the visualization displays two input signals (typically the left and right channel) plotted against each other, with one signal driving the horizontal axis (X)
and the other the vertical axis (Y), instead of the usual time-based sweep.

This mode is useful for visualizing the relationship between two signals, such as phase differences, frequency ratios, or specific patterns like Lissajous figures.

This visualization uses the following settings:

- Graph X-axis and Y-axis settings
  - Select *None* to prevent the axis from being rendered.
- Channel pair selection
- X and Y signal gain
- Phosphor effect settings

![Screenshot](assets/Oscilloscope-Lissajous.png?raw=true "Screenshot")

<sup>Oscilloscope in X-Y mode displaying a Lissajous figure</sup>

#### Peak indicators group

Some visualizations can display indicators for the peak values. The following settings determine how those peak indicators are animated.

Set the visualization type to **Bars**, **Peak / RMS** or **Balance / Correlation** to enable these settings.

`Peak mode`

Specifies how the peak indicators are rendered:

- None
- Classic
- Gravity
- AIMP
- Fade Out
- Fading AIMP: A combination of AIMP and Fade Out

`Hold time`

Specifies how long a peak value will be held steady before it decays.

`Acceleration`

Specifies the acceleration used to decay the peak value.

#### LEDs group

Some visualizations can display a bar as simulated LED lights.

Set the visualization type to **Bars**, **Peak / RMS** or **Balance / Correlation** to enable these settings.

`Enabled`

Display the spectrum bars and peak meters as LEDs.

`LED size`

The size of the LED light, in pixels.

`LED gap`

The size of the gap between the LEDs, in pixels.

#### Spectogram group

Set the visualization type to **Spectogram** to enable these settings.

`Scrolling`

Activates scrolling of the spectogram.

`Horizontal`

Renders the spectogram horizontally when enabled, vertically when not.

`Use spectrum bar metrics`

#### Radial Bars group

Set the visualization type to **Radial Bars** to enable these settings.

`Inner radius`

Sets the inner radius as a percentage of the smallest side of the graph.

`Outer radius`

Sets the outer radius as a percentage of the smallest side of the graph.

> [!Note]
> If the inner radius is greater than the outer radius the bars are drawn inwards.

`Angular velocity`

Sets the angular velocity of the rotation in degrees per second. Use positive values for clockwise rotation; negative values for anti-clockwise rotation.

#### Level Meter group

Set the visualization type to **Balance / Correlation** to enable these settings.

`Channel pairs`

Allows you to select the channel pair that will be visualized.

`Horizontal`

Renders the level meter horizontally when enabled, vertically when not.

#### Peak Meter group

The peak meter will display the instant peak and RMS over time level of the playing track.

> [!Tip]
> A gauge will be shown for each of the channels in the playing track but only when it is also selected in the Channels list on the Graphs page. F.e. a 7.1 track with only the Front Left and Front Right channel selected will only show 2 gauges. A mono track without the Front Center channel selected will show no gauge.

Set the visualization type to **Peak / RMS** to enable these settings.

`Horizontal`

Renders the peak meter horizontally when enabled, vertically when not.

`RMS+3`

Adds 3dB to the RMS value.

`RMS window`

Specifies the duration of each RMS measurement in seconds.

`Gauge gap`

Specifies the gap between the gauges (in pixels). Defaults to 1 pixel.

---

### Graphs page

The component window can contain more than one graph at the same time. Some users use this to create facing spectrum analyses. Graphs can have individual settings but share the visualization type and the styles.

`Graph list`

The graph list shows all graphs in the current window. Use the + button to add a graph. Use the - button to remove the selected graph.

`Vertical layout`

Enables this setting to stack the graphs vertically instead of horizontally.

`Description`

Allow you to edit the description of the selected graph.

#### Layout group

`Horizontal alignment`

Determines the horizontal alignment of a graph within its bounds (taking into account any space used by the X axis).

- *Near* aligns the visualization with the near side of the graph (aka the *left* edge when using normal orientation).
- *Center* centers the visualization in the graph area.
- *Far* aligns the visualization with the far side of the graph (aka the *right* edge when using normal orientation).
- *Fit* makes the visualization use the full horizontal the graph area.

`Flip horizontally`

Flips the current graph horizontally. Any axes will be rendered accordingly.

`Flip vertically`

Flips the current graph vertically. Any axes will be rendered accordingly.

#### X axis group

Groups the parameters that determine the way the X axis is displayed.

`Mode`

Determines which X axis to display.

- None: Hides the X axis (reserve no screen area for it).
- Bands: Center frequency of a band, every 10 bands.
- Decades: Fixed frequency range
- Octaves: Frequency of the C note of each octave
- Notes: C note of each octave

`Top`

Displays an X-axis on top of the graph.

`Bottom`

Displays an X-axis at the bottom of the graph.

#### Y axis group

Groups the parameters that determine the way the Y axis is displayed.

`Mode`

Determines which Y axis to display.

- None: Hides the Y axis (reserve no screen area for it).
- Decibel: Uses decibel values to render the scale.
- Linear/n-th root: Uses logarithmic values to render the scale.

`Left`

Displays an Y-axis on the left of the graph.

`Right`

Displays an Y-axis on the bottom of the graph.

`Amplitude range`

Determines the minimum and maximum amplitude, expressed in decibel (dB), to display.

`Amplitude step`

Determine the step size between the min. and max. amplitude.

`Use absolute`

Sets the min. amplitude to -∞ dB (0.0 on the linear scale) when enabled.

`Gamma`

Sets index n of the n-th root calculation.

`Channels`

Allows you to select the audio channels that will be used to participate in the analysis of an audio frame.

> [!Tip]
> Most users will enable only the **Left** and the **Right** channel. Therefor no information will be shown in the graph when a mono track is played. Also enable the **Front Center** channel to remedy this situation.

---

### Styles page

Most of the elements of a graph can be styled. The styles list shows all available styles. Each style has a number of settings that determine how the element is rendered.

`Color source`

The color source determines where the element gets its color information from.

- None: Prevents the element from being rendered.
- Solid: A single color is used to render the element.
- Dominant color: The dominant color in the artwork is used to render the element. The artwork does not have to be rendered on the background.
- Gradient: The element is rendered using a gradient created from the color list below.
- Windows: The element is rendered using a standard Windows color.
- User Interface: The element is rendered using a standard DUI or CUI user interface color.

`Color index`

Allow you to select a color when color source Window or User Interface is specified.

`Color`

Displays the current color used by color source Solid or Dominant Color. Click to modify the color.

> [!Tip]
> A standard Windows color dialog that has been extended with controls to modify the alpha channel of the color will pop up.

`Color scheme`

Specifies the color scheme used to render the element.

- Solid color: A single color, effectively the same as color source Solid.
- Custom: A custom list of colors.
- Artwork: A list of colors determined by the current artwork
- Prism 1
- Prism 2
- Prism 3
- foobar2000
- foobar2000 Dark Mode
- Fire
- Rainbow
- SoX: A list of colors emulating the spectogram colors used by SoX.

`Color list`

Double-click a color to modify it. Modifying a color of a built-in color scheme automatically selects the Custom color scheme.

Press the ''Add'' button to add a color, ''Remove'' to remove a color, ''Reverse'' to reverse the color list.

The position of the color in the gradient is expressed as a percentage of the length of the gradient. E.g. a position of 50% puts the color in the middle of the gradient.

Press the ''Spread'' button to evenly spread the colors in the list over the gradient. This updates the position of the colors as required.

`Horizontal gradient`

Generates a horizontal instead of a vertical gradient.

`Amplitude-based`

Some styles can use the amplitude of a spectrum frequency to determine the color. This option can only be applied to a horizontal gradient.

`Opacity`

Specifies the opacity of the rendered element.

`Thickness`

For some styles you can specify the thickness of the brush used to render the element.

`Font name`

Specifies the name of the font used to render the element.

`Font size`

Specifies the size in points of the font used to render the element.

---

### Presets page

The whole configuration can be saved to a preset. A preset can be reloaded at any time.

> [!Note]
> Presets are not interchangeable with x86 and x64 versions of foobar2000. Special care has been taken to be as backwards compatible with preset files saved by older versions of the component.

You can recognize preset files by the `fvsa` file extension.

`Location`

Contains the path name of the location of your preset files. By default the foobar2000 profile directory is used.

> [!Note]
> For privacy reasons the location is stored in your foobar2000 settings but will not be included in a preset file. Preset files contain no personal information and can be safely exchanged with other users.

Below the location you find the preset files currently found in the specified location.

Double click a preset to activate it or select it and press the Load button.

`Name`

Specifies the name of the preset.

`Load`

Loads and activates the currently selected preset.

`Save`

Saves the current configuration.

`Delete`

Deletes the currently selected preset.

---

## Troubleshooting

🔧 *Work in Progress*

---

## FAQs

**Q:** Why does the fullscreen version of the component not respect my settings when using DUI?
**A:** In DUI the fullscreen version of the component is a completely new instance with its own settings. You have to configure it separately.

---

## Reference Material

This chapter contains a lot of reference material I consulted during the development of foo_vis_spectrum_analyzer.

### foobar2000

- [foobar2000 Development](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Overview)

### Windows User Interface

- [Desktop App User Interface](https://learn.microsoft.com/en-us/windows/win32/windows-application-ui-development)
- [Windows User Experience Interaction Guidelines](https://learn.microsoft.com/en-us/windows/win32/uxguide/guidelines)
- [Windows Controls](https://learn.microsoft.com/en-us/windows/win32/controls/window-controls)
- [Control Library](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info)
- [Resource-Definition Statements](https://learn.microsoft.com/en-us/windows/win32/menurc/resource-definition-statements)
- [Visuals, Layout](https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-layout)

## Transforms

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

---

## History

The history of foo_vis_spectrum_analyzer development is available in a separate [document](History.md).

---

## Acknowledgements and Credits

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

---

## Support

For further assistance:

🌐 Home page: [https://github.com/stuerp/foo_vis_spectrum_analyzer](https://github.com/stuerp/foo_vis_spectrum_analyzer)  
🌐 Issue tracker: [https://github.com/stuerp/foo_vis_spectrum_analyzer/issues](https://github.com/stuerp/foo_vis_spectrum_analyzer/issues)  
🌐 Forum: [Hydrogenaudio foo_vis_spectrum_analyzer](https://hydrogenaud.io/index.php/topic,125031.0.html)
