
/** $VER: Analysis.cpp (2024.02.21) P. Stuer **/

#include "Analysis.h"

#include "Support.h"

#pragma hdrstop

inline double GetFrequencyTilt(double x, double amount, double offset) noexcept;
inline double Equalize(double x, double amount, double depth, double offset) noexcept;
inline double GetAcousticWeight(double x, WeightingType weightingType, double weightAmount) noexcept;

/// <summary>
/// Initializes this instance.
/// </summary>
void Analysis::Initialize(const State * state, const GraphSettings * settings) noexcept
{
    _State = state;
    _GraphSettings = settings;

    if (state->_Transform == Transform::FFT)
    {
        switch (state->_FrequencyDistribution)
        {
            default:

            case FrequencyDistribution::Linear:
                GenerateLinearFrequencyBands(state);
                break;

            case FrequencyDistribution::Octaves:
                GenerateOctaveFrequencyBands(state);
                break;

            case FrequencyDistribution::AveePlayer:
                GenerateAveePlayerFrequencyBands(state);
                break;
        }
    }
    else
        GenerateOctaveFrequencyBands(state);
}

/// <summary>
/// Processes the audio samples.
/// </summary>
void Analysis::Process(const audio_chunk & chunk) noexcept
{
    const audio_sample * Samples = chunk.get_data();

    if (Samples == nullptr)
        return;

    const size_t SampleCount = chunk.get_sample_count();

    _SampleRate = chunk.get_sample_rate();

    GetAnalyzer(chunk);

    switch (_State->_Transform)
    {
        case Transform::FFT:
        {
            _FFTAnalyzer->AnalyzeSamples(Samples, SampleCount, _GraphSettings->_Channels, _FrequencyBands);
            break;
        }

        case Transform::CQT:
        {
            _CQTAnalyzer->AnalyzeSamples(Samples, SampleCount, _GraphSettings->_Channels, _FrequencyBands);
            break;
        }

        case Transform::SWIFT:
        {
            _SWIFTAnalyzer->AnalyzeSamples(Samples, SampleCount, _GraphSettings->_Channels, _FrequencyBands);
            break;
        }
    }

    // Filter the spectrum.
    if (_State->_WeightingType != WeightingType::None)
        ApplyAcousticWeighting();

    // Smooth the spectrum.
    switch (_State->_SmoothingMethod)
    {
        default:

        case SmoothingMethod::None:
        {
            ApplyAverageSmoothing(0.);
            break;
        }

        case SmoothingMethod::Average:
        {
            ApplyAverageSmoothing(_State->_SmoothingFactor);
            break;
        }

        case SmoothingMethod::Peak:
        {
            ApplyPeakSmoothing(_State->_SmoothingFactor);
            break;
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void Analysis::Reset()
{
    if (_SWIFTAnalyzer != nullptr)
    {
        delete _SWIFTAnalyzer;
        _SWIFTAnalyzer = nullptr;
    }

    if (_CQTAnalyzer != nullptr)
    {
        delete _CQTAnalyzer;
        _CQTAnalyzer = nullptr;
    }

    if (_FFTAnalyzer != nullptr)
    {
        delete _FFTAnalyzer;
        _FFTAnalyzer = nullptr;
    }

    if (_BrownPucketteKernel != nullptr)
    {
        delete _BrownPucketteKernel;
        _BrownPucketteKernel = nullptr;
    }

    if (_WindowFunction != nullptr)
    {
        delete _WindowFunction;
        _WindowFunction = nullptr;
    }
}

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void Analysis::GenerateLinearFrequencyBands(const State * state)
{
    const double MinFreq = ScaleF(state->_LoFrequency, state->_ScalingFunction, state->_SkewFactor);
    const double MaxFreq = ScaleF(state->_HiFrequency, state->_ScalingFunction, state->_SkewFactor);

    const double Bandwidth = (((state->_Transform == Transform::FFT) && (state->_MappingMethod == Mapping::TriangularFilterBank)) || (state->_Transform == Transform::CQT)) ? state->_Bandwidth : 0.5;

    _FrequencyBands.resize(state->_BandCount);

    double i = 0.;

    for (FrequencyBand & Iter: _FrequencyBands)
    {
        Iter.Lo  = DeScaleF(Map(i - Bandwidth, 0., (double)(state->_BandCount - 1), MinFreq, MaxFreq), state->_ScalingFunction, state->_SkewFactor);
        Iter.Ctr = DeScaleF(Map(i,             0., (double)(state->_BandCount - 1), MinFreq, MaxFreq), state->_ScalingFunction, state->_SkewFactor);
        Iter.Hi  = DeScaleF(Map(i + Bandwidth, 0., (double)(state->_BandCount - 1), MinFreq, MaxFreq), state->_ScalingFunction, state->_SkewFactor);

        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);

        Iter.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void Analysis::GenerateOctaveFrequencyBands(const State * state)
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (state->_Pitch > 0.) ? ::round((::log2(state->_Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = state->_Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NoteGroup = 24. / state->_BandsPerOctave;

    const double LoNote = ::round(state->_MinNote * 2. / NoteGroup);
    const double HiNote = ::round(state->_MaxNote * 2. / NoteGroup);

    const double Bandwidth = (((state->_Transform == Transform::FFT) && (state->_MappingMethod == Mapping::TriangularFilterBank)) || (state->_Transform == Transform::CQT)) ? state->_Bandwidth : 0.5;

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, (i - Bandwidth) * NoteGroup + state->_Transpose),
            C0 * ::pow(Root24,  i              * NoteGroup + state->_Transpose),
            C0 * ::pow(Root24, (i + Bandwidth) * NoteGroup + state->_Transpose),
        };

        // Pre-calculate the tooltip text and the band background color.
        {
            int Note = (int) (i * (NoteGroup / 2.));

            int n = Note % 12;

            ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteName[n], Note / 12, fb.Ctr);

            fb.HasDarkBackground = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        }

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands like AveePlayer.
/// </summary>
void Analysis::GenerateAveePlayerFrequencyBands(const State * state)
{
    const double Bandwidth = (((state->_Transform == Transform::FFT) && (state->_MappingMethod == Mapping::TriangularFilterBank)) || (state->_Transform == Transform::CQT)) ? state->_Bandwidth : 0.5;

    _FrequencyBands.resize(state->_BandCount);

    double i = 0.;

    for (FrequencyBand & Iter : _FrequencyBands)
    {
        Iter.Lo  = LogSpace(state->_LoFrequency, state->_HiFrequency, i - Bandwidth, state->_BandCount - 1, state->_SkewFactor);
        Iter.Ctr = LogSpace(state->_LoFrequency, state->_HiFrequency, i,             state->_BandCount - 1, state->_SkewFactor);
        Iter.Hi  = LogSpace(state->_LoFrequency, state->_HiFrequency, i + Bandwidth, state->_BandCount - 1, state->_SkewFactor);

        Iter.HasDarkBackground = true;
        ::swprintf_s(Iter.Label, _countof(Iter.Label), L"%.2fHz", Iter.Ctr);

        ++i;
    }
}

/// <summary>
/// Gets an analyzer and any supporting objects.
/// </summary>
void Analysis::GetAnalyzer(const audio_chunk & chunk) noexcept
{
    if (_WindowFunction == nullptr)
        _WindowFunction = WindowFunction::Create(_State->_WindowFunction, _State->_WindowParameter, _State->_WindowSkew, _State->_Truncate);

    if (_BrownPucketteKernel == nullptr)
        _BrownPucketteKernel = WindowFunction::Create(_State->_KernelShape, _State->_KernelShapeParameter, _State->_KernelAsymmetry, _State->_Truncate);

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    if ((_FFTAnalyzer == nullptr) && (_State->_Transform == Transform::FFT))
    {
        _FFTAnalyzer = new FFTAnalyzer(_State, _SampleRate, ChannelCount, ChannelSetup, *_WindowFunction, *_BrownPucketteKernel, _State->_BinCount);
    }

    if ((_CQTAnalyzer == nullptr) && (_State->_Transform == Transform::CQT))
    {
        _CQTAnalyzer = new CQTAnalyzer(_State, _SampleRate, ChannelCount, ChannelSetup, *_WindowFunction);
    }

    if ((_SWIFTAnalyzer == nullptr) && (_State->_Transform == Transform::SWIFT))
    {
        _SWIFTAnalyzer = new SWIFTAnalyzer(_State, _SampleRate, ChannelCount, ChannelSetup);

        _SWIFTAnalyzer->Initialize(_FrequencyBands);
    }
}

#pragma region Acoustic Weighting

/// <summary>
/// Applies acoustic weighting to the spectrum.
/// </summary>
void Analysis::ApplyAcousticWeighting()
{
    const double Offset = ((_State->_SlopeFunctionOffset * (double) _SampleRate) / (double) _State->_BinCount);

    for (FrequencyBand & Iter : _FrequencyBands)
        Iter.NewValue *= GetWeight(Iter.Ctr + Offset);
}

/// <summary>
/// Gets the total weight.
/// </summary>
double Analysis::GetWeight(double x) const noexcept
{
    const double a = GetFrequencyTilt(x, _State->_Slope, _State->_SlopeOffset);
    const double b = Equalize(x, _State->_EqualizeAmount, _State->_EqualizeDepth, _State->_EqualizeOffset);
    const double c = GetAcousticWeight(x, _State->_WeightingType, _State->_WeightingAmount);

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
    const double bias = ::pow(1.0025, -pos) * 0.04;

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

#pragma endregion

/// <summary>
/// Smooths the spectrum using averages.
/// </summary>
void Analysis::ApplyAverageSmoothing(double factor) noexcept
{
    if (factor != 0.0)
    {
        for (FrequencyBand & fb : _FrequencyBands)
            fb.CurValue = (::isfinite(fb.CurValue) ? fb.CurValue * factor : 0.0) + (::isfinite(fb.NewValue) ? fb.NewValue * (1.0 - factor) : 0.0);

    }
    else
    {
        for (FrequencyBand & fb : _FrequencyBands)
            fb.CurValue = fb.NewValue;
    }
}

/// <summary>
/// Smooths the spectrum using peak decay.
/// </summary>
void Analysis::ApplyPeakSmoothing(double factor) noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = Max(::isfinite(fb.CurValue) ? fb.CurValue * factor : 0.0, ::isfinite(fb.NewValue) ? fb.NewValue : 0.0);
}

/// <summary>
/// Updates the value of the peak indicators.
/// </summary>
void Analysis::UpdatePeakIndicators() noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
    {
        const double Amplitude = Clamp(_GraphSettings->ScaleA(fb.CurValue), 0., 1.);

        if (Amplitude >= fb.Peak)
        {
            if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                fb.HoldTime = (::isfinite(fb.HoldTime) ? fb.HoldTime : 0.) + (Amplitude - fb.Peak) * _State->_HoldTime;
            else
                fb.HoldTime = _State->_HoldTime;

            fb.Peak = Amplitude;
            fb.DecaySpeed = 0.;
            fb.Opacity = 1.;
        }
        else
        {
            if (fb.HoldTime >= 0.)
            {
                if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                    fb.Peak += (fb.HoldTime - Max(fb.HoldTime - 1., 0.)) / _State->_HoldTime;

                fb.HoldTime -= 1.;

                if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                    fb.HoldTime = Min(fb.HoldTime, _State->_HoldTime);
            }
            else
            {
                switch (_State->_PeakMode)
                {
                    default:

                    case PeakMode::None:
                        break;

                    case PeakMode::Classic:
                        fb.DecaySpeed = _State->_Acceleration / 256.;
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::Gravity:
                        fb.DecaySpeed += _State->_Acceleration / 256.;
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::AIMP:
                        fb.DecaySpeed = (_State->_Acceleration / 256.) * (1. + (int) (fb.Peak < 0.5));
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::FadeOut:
                        fb.DecaySpeed += _State->_Acceleration / 256.;
                        fb.Opacity -= fb.DecaySpeed;

                        if (fb.Opacity <= 0.)
                            fb.Peak = Amplitude;
                        break;

                    case PeakMode::FadingAIMP:
                        fb.DecaySpeed = (_State->_Acceleration / 256.) * (1. + (int) (fb.Peak < 0.5));
                        fb.Peak -= fb.DecaySpeed;
                        fb.Opacity -= fb.DecaySpeed;

                        if (fb.Opacity <= 0.)
                            fb.Peak = Amplitude;
                        break;
                }
            }

            fb.Peak = Clamp(fb.Peak, 0., 1.);
        }
    }
}
