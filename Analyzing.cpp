
/** $VER: Analyzing.cpp (2024.01.22) P. Stuer **/

#include "UIElement.h"

#include "Log.h"

#pragma hdrstop

inline double GetFrequencyTilt(double x, double amount = 3., double offset = 1000.) noexcept;
inline double Equalize(double x, double amount = 6., double depth = 1024., double offset = 44100.) noexcept;
inline double GetAcousticWeight(double x, WeightingType = WeightingType::AWeighting, double weightAmount = 1.) noexcept;

/// <summary>
/// Processes an audio chunk.
/// </summary>
void UIElement::ProcessAudioChunk(const audio_chunk & chunk) noexcept
{
    _SampleRate = chunk.get_sample_rate();

    GetAnalyzer(chunk);

    // Get the spectrum.
    {
        const audio_sample * Samples = chunk.get_data();

        if (Samples == nullptr)
            return;

        size_t SampleCount = chunk.get_sample_count();

        if (_Configuration._Transform == Transform::FFT)
        {
            _FFTAnalyzer->Add(Samples, SampleCount, _Configuration._SelectedChannels);

            _FFTAnalyzer->GetFrequencyCoefficients(_FrequencyCoefficients);

            if (_Configuration._MappingMethod == Mapping::Standard)
                _FFTAnalyzer->GetFrequencyBands(_FrequencyCoefficients, _SampleRate, _Configuration._SummationMethod, _FrequencyBands);
            else
                _FFTAnalyzer->GetFrequencyBands(_FrequencyCoefficients, _SampleRate, _FrequencyBands);
        }
        else
            _CQTAnalyzer->GetFrequencyBands(Samples, SampleCount, _Configuration._SelectedChannels, _FrequencyBands);
    }

    // Filter the spectrum.
    ApplyAcousticWeighting();

    // Smooth the spectrum.
    switch (_Configuration._SmoothingMethod)
    {
        default:

        case SmoothingMethod::Average:
        {
            ApplyAverageSmoothing(_Configuration._SmoothingFactor);
            break;
        }

        case SmoothingMethod::Peak:
        {
            ApplyPeakSmoothing(_Configuration._SmoothingFactor);
            break;
        }
    }
}

/// <summary>
/// Gets an analyzer and any supporting objects.
/// </summary>
void UIElement::GetAnalyzer(const audio_chunk & chunk) noexcept
{
    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    if (_WindowFunction == nullptr)
        _WindowFunction = WindowFunction::Create(_Configuration._WindowFunction, _Configuration._WindowParameter, _Configuration._WindowSkew, _Configuration._Truncate);

    if (_FFTAnalyzer == nullptr)
    {
        _FFTAnalyzer = new FFTAnalyzer(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, _FFTSize, &_Configuration);

        _FrequencyCoefficients.resize(_FFTSize);
    }

    if (_CQTAnalyzer == nullptr)
        _CQTAnalyzer = new CQTAnalyzer(ChannelCount, ChannelSetup, (double) _SampleRate, *_WindowFunction, 1.0, 1.0, 0.0, &_Configuration);
}

/// <summary>
/// Generates frequency bands.
/// </summary>
void UIElement::GenerateLinearFrequencyBands()
{
    const double MinFreq = ScaleF(_Configuration._LoFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);
    const double MaxFreq = ScaleF(_Configuration._HiFrequency, _Configuration._ScalingFunction, _Configuration._SkewFactor);

    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = DeScaleF(Map((double) i - _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Ctr = DeScaleF(Map((double) i,              0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);
        Iter.Hi  = DeScaleF(Map((double) i + _Bandwidth, 0., (double)(_Configuration._NumBands - 1), MinFreq, MaxFreq), _Configuration._ScalingFunction, _Configuration._SkewFactor);

        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void UIElement::GenerateOctaveFrequencyBands()
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (_Configuration._Pitch > 0.) ? ::round((::log2(_Configuration._Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = _Configuration._Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NotesGroup = 24. / _Configuration._BandsPerOctave;

    const double LoNote = ::round(_Configuration._MinNote * 2. / NotesGroup);
    const double HiNote = ::round(_Configuration._MaxNote * 2. / NotesGroup);

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, ((i - _Bandwidth) * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24,  (i               * NotesGroup + _Configuration._Transpose)),
            C0 * ::pow(Root24, ((i + _Bandwidth) * NotesGroup + _Configuration._Transpose)),
        };

        int Note = (int) (i * (NotesGroup / 2.));

        int n = Note % 12;

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], Note / 12, fb.Ctr);

        fb.BackColor = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10) ? _Configuration._DarkBandColor : _Configuration._LightBandColor;

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands of AveePlayer.
/// </summary>
void UIElement::GenerateAveePlayerFrequencyBands()
{
    _FrequencyBands.resize(_Configuration._NumBands);

    for (size_t i = 0; i < _FrequencyBands.size(); ++i)
    {
        FrequencyBand& Iter = _FrequencyBands[i];

        Iter.Lo  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i - _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Ctr = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i,              _Configuration._NumBands - 1, _Configuration._SkewFactor);
        Iter.Hi  = LogSpace(_Configuration._LoFrequency, _Configuration._HiFrequency, (double) i + _Bandwidth, _Configuration._NumBands - 1, _Configuration._SkewFactor);

        Iter.BackColor = _Configuration._DarkBandColor;
    }
}

/// <summary>
/// Scales the frequency.
/// </summary>
double UIElement::ScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::log2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::Mel:
            return ::log2(1.0 + x / 700.0);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * x) / (1960.0 + x) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * x) / (::pow(10, skewFactor * 4.0) + x);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * x);

        case ScalingFunction::Cams:
            return ::log2((x / 1000.0 + 0.312) / (x / 1000.0 + 14.675));

        case ScalingFunction::HyperbolicSine:
            return ::asinh(x / ::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return ::pow(x, (1.0 / (11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::exp2(-x / ::exp2(7 + skewFactor * 8));

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Descales the frequency.
/// </summary>
double UIElement::DeScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case ScalingFunction::Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case ScalingFunction::AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case ScalingFunction::Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case ScalingFunction::HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case ScalingFunction::NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// Applies acoustic weighting to the spectrum.
/// </summary>
void UIElement::ApplyAcousticWeighting()
{
    const double Offset = ((_Configuration._SlopeFunctionOffset * (double) _SampleRate) / (double) _FFTSize);

    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue *= GetWeight(Iter.Ctr + Offset);
}

/// <summary>
/// Gets the total weight.
/// </summary>
double UIElement::GetWeight(double x) const noexcept
{
    const double a = GetFrequencyTilt(x, _Configuration._Slope, _Configuration._SlopeOffset);
    const double b = Equalize(x, _Configuration._EqualizeAmount, _Configuration._EqualizeDepth, _Configuration._EqualizeOffset);
    const double c = GetAcousticWeight(x, _Configuration._WeightingType, _Configuration._WeightingAmount);

    return a * b * c;
}

/// <summary>
/// Gets the frequency tilt.
/// </summary>
double GetFrequencyTilt(double x, double amount, double offset) noexcept
{
    return ::pow(x / offset, amount / 6.);
}

/// <summary>
/// Equalizes the weight.
/// </summary>
double Equalize(double x, double amount, double depth, double offset) noexcept
{
    const double pos = x * depth / offset;
    const double bias = ::pow(1.0025, -pos * 0.04);

    return ::pow((10. * ::log10(1. + bias + (pos + 1.) * (9. - bias) / depth)), amount / 6.);
}

/// <summary>
/// Gets the weight for the specified frequency.
/// </summary>
double GetAcousticWeight(double x, WeightingType weightType, double weightAmount) noexcept
{
    const double f2 = x * x;

    switch (weightType)
    {
        default:

        case WeightingType::None:
            return 1.;

        case WeightingType::AWeighting:
            return ::pow(1.2588966          * 148840000 * (f2 * f2)    / ((f2 + 424.36) * ::sqrt((f2 + 11599.29) * (f2 + 544496.41)) * (f2 + 148840000.)), weightAmount);

        case WeightingType::BWeighting:
            return ::pow(1.019764760044717  * 148840000 * ::pow(x, 3.) / ((f2 + 424.36) * ::sqrt(f2 + 25122.25)                      * (f2 + 148840000.)), weightAmount);

        case WeightingType::CWeighting:
            return ::pow(1.0069316688518042 * 148840000 * f2           / ((f2 + 424.36)                                              * (f2 + 148840000.)), weightAmount);

        case WeightingType::DWeighting:
            return ::pow(x / 6.8966888496476e-5 * ::sqrt(((1037918.48 - f2) * (1037918.48 - f2) + 1080768.16 * f2) / ((9837328. - f2) * (9837328. - f2) + 11723776. * f2) / ((f2 + 79919.29) * (f2 + 1345600.))), weightAmount);

        case WeightingType::MWeighting:
        {
            const double h1 = -4.737338981378384e-24 * ::pow(f2, 3.) + 2.043828333606125e-15 * (f2 * f2)    - 1.363894795463638e-7 * f2 + 1;
            const double h2 =  1.306612257412824e-19 * ::pow( x, 5.) - 2.118150887518656e-11 * ::pow(x, 3.) + 5.559488023498642e-4 * x;

            return ::pow(8.128305161640991 * 1.246332637532143e-4 * x / ::hypot(h1, h2), weightAmount);
        }
    }
}

/// <summary>
/// Smooths the spectrum using averages.
/// </summary>
void UIElement::ApplyAverageSmoothing(double factor)
{
    if (factor != 0.0)
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = (::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0) + (::isfinite(Iter.NewValue) ? Iter.NewValue * (1.0 - factor) : 0.0);

    }
    else
    {
        for (FrequencyBand & Iter : _FrequencyBands)
            Iter.CurValue = Iter.NewValue;
    }
}

/// <summary>
/// Smooths the spectrum using peak decay.
/// </summary>
void UIElement::ApplyPeakSmoothing(double factor)
{
    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.CurValue = Max(::isfinite(Iter.CurValue) ? Iter.CurValue * factor : 0.0, ::isfinite(Iter.NewValue) ? Iter.NewValue : 0.0);
}
