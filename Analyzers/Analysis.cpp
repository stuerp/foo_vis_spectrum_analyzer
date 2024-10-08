
/** $VER: Analysis.cpp (2024.08.18) P. Stuer **/

#include "framework.h"

#include "Analysis.h"
#include "Log.h"

#include "Support.h"

#pragma hdrstop

inline double GetFrequencyTilt(double x, double amount, double offset) noexcept;
inline double Equalize(double x, double amount, double depth, double offset) noexcept;
inline double GetAcousticWeight(double x, WeightingType weightingType, double weightAmount) noexcept;

/// <summary>
/// Initializes this instance.
/// </summary>
void Analysis::Initialize(const state_t * threadState, const GraphSettings * settings) noexcept
{
    _State = threadState;
    _GraphSettings = settings;

    switch (threadState->_FrequencyDistribution)
    {
        default:

        case FrequencyDistribution::Linear:
            GenerateLinearFrequencyBands();
            break;

        case FrequencyDistribution::Octaves:
            GenerateOctaveFrequencyBands();
            break;

        case FrequencyDistribution::AveePlayer:
            GenerateAveePlayerFrequencyBands();
            break;
    }

    Reset();
}

/// <summary>
/// Processes the audio samples.
/// </summary>
void Analysis::Process(const audio_chunk & chunk) noexcept
{
    switch (_State->_VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectogram:
        case VisualizationType::RadialBars:
        {
            const audio_sample * Samples = chunk.get_data();
            const size_t SampleCount = chunk.get_sample_count();

            if ((Samples == nullptr) || (SampleCount == 0))
                return;

            _SampleRate = chunk.get_sample_rate();
            _NyquistFrequency = (double) _SampleRate / 2.;

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

                case Transform::AnalogStyle:
                {
                    _AnalogStyleAnalyzer->AnalyzeSamples(Samples, SampleCount, _GraphSettings->_Channels, _FrequencyBands);
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
                    Normalize();
                    break;
                }

                case SmoothingMethod::Average:
                {
                    NormalizeWithAverageSmoothing(_State->_SmoothingFactor);
                    break;
                }

                case SmoothingMethod::Peak:
                {
                    NormalizeWithPeakSmoothing(_State->_SmoothingFactor);
                    break;
                }
            }

            // From here one CurValue is guaranteed in the range 0.0 .. 1.0
        }
  
        case VisualizationType::PeakMeter:
        case VisualizationType::LevelMeter:
        {
            GetGaugeValues(chunk);
            break;
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void Analysis::Reset()
{
    if (_AnalogStyleAnalyzer != nullptr)
    {
        delete _AnalogStyleAnalyzer;
        _AnalogStyleAnalyzer = nullptr;
    }

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

    {
        _CurrentChannelMask = 0;
        InitializeGauges((uint32_t) Channel::ConfigStereo);
    }

    {
        _RMSTimeElapsed = 0.;
        _RMSSampleCount = 0;

        _Left  = 0.;
        _Right = 0.;

        _Mid  = 0.;
        _Side = 0.;

        _Balance = 0.5;
        _Phase   = 0.5;
    }
}

/// <summary>
/// Updates the peak values.
/// </summary>
void Analysis::UpdatePeakValues(bool isStopped) noexcept
{
    const double Acceleration = _State->_Acceleration / 256.;

    switch (_State->_VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectogram:
        {
            // Animate the spectrum peak value.
            for (auto & fb : _FrequencyBands)
            {
                if (fb.CurValue >= fb.MaxValue)
                {
                    if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                        fb.HoldTime = (::isfinite(fb.HoldTime) ? fb.HoldTime : 0.) + (fb.CurValue - fb.MaxValue) * _State->_HoldTime;
                    else
                        fb.HoldTime = _State->_HoldTime;

                    fb.MaxValue = fb.CurValue;
                    fb.DecaySpeed = 0.;
                    fb.Opacity = 1.;
                }
                else
                {
                    if (fb.HoldTime >= 0.)
                    {
                        if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                            fb.MaxValue += (fb.HoldTime - std::max(fb.HoldTime - 1., 0.)) / _State->_HoldTime;

                        fb.HoldTime--;

                        if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                            fb.HoldTime = std::min(fb.HoldTime, _State->_HoldTime);
                    }
                    else
                    {
                        switch (_State->_PeakMode)
                        {
                            default:

                            case PeakMode::None:
                                break;

                            case PeakMode::Classic:
                                fb.DecaySpeed = Acceleration;
                                fb.MaxValue   -= fb.DecaySpeed;
                                break;

                            case PeakMode::Gravity:
                                fb.DecaySpeed += Acceleration;
                                fb.MaxValue   -= fb.DecaySpeed;
                                break;

                            case PeakMode::AIMP:
                                fb.DecaySpeed = Acceleration * (1. + (int) (fb.MaxValue < 0.5));
                                fb.MaxValue  -= fb.DecaySpeed;
                                break;

                            case PeakMode::FadeOut:
                                fb.DecaySpeed += Acceleration;

                                fb.Opacity -= fb.DecaySpeed;

                                if (fb.Opacity <= 0.)
                                    fb.MaxValue = fb.CurValue;
                                break;

                            case PeakMode::FadingAIMP:
                                fb.DecaySpeed = Acceleration * (1. + (int) (fb.MaxValue < 0.5));
                                fb.MaxValue  -= fb.DecaySpeed;

                                fb.Opacity -= fb.DecaySpeed;

                                if (fb.Opacity <= 0.)
                                    fb.MaxValue = fb.CurValue;
                                break;
                        }
                    }

                    fb.MaxValue = std::clamp(fb.MaxValue, 0., 1.);
                }
            }
            break;
        }

        case VisualizationType::PeakMeter:
        {
            // Animate the smoothed peak and RMS values.
            for (auto & gv : _GaugeValues)
            {
                if (gv.PeakRender >= gv.MaxPeakRender)
                {
                    if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                        gv.HoldTime = (::isfinite(gv.HoldTime) ? gv.HoldTime : 0.) + (gv.PeakRender - gv.MaxPeakRender) * _State->_HoldTime;
                    else
                        gv.HoldTime = _State->_HoldTime;

                    gv.MaxPeakRender = gv.PeakRender;
                    gv.DecaySpeed = 0.;
                    gv.Opacity = 1.;
                }
                else
                {
                    if (gv.HoldTime > 0.)
                    {
                        if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                        {
                            gv.MaxPeakRender += (gv.HoldTime - std::max(gv.HoldTime - 1., 0.)) / _State->_HoldTime;
                        }

                        gv.HoldTime--;

                        if ((_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP))
                            gv.HoldTime = std::min(gv.HoldTime, _State->_HoldTime);
                    }
                    else
                    {
                        switch (_State->_PeakMode)
                        {
                            default:

                            case PeakMode::None:
                                break;

                            case PeakMode::Classic:
                                gv.DecaySpeed = Acceleration;

                                gv.MaxPeakRender -= gv.DecaySpeed;
                                break;

                            case PeakMode::Gravity:
                                gv.DecaySpeed += Acceleration;

                                gv.MaxPeakRender -= gv.DecaySpeed;
                                break;

                            case PeakMode::AIMP:
                                gv.DecaySpeed = Acceleration * (1. + (int) (gv.MaxPeakRender < 0.5));

                                gv.MaxPeakRender -= gv.DecaySpeed;
                                break;

                            case PeakMode::FadeOut:
                                gv.DecaySpeed += Acceleration;

                                gv.Opacity -= gv.DecaySpeed;

                                if (gv.Opacity <= 0.)
                                    gv.MaxPeakRender = gv.PeakRender;
                                break;

                            case PeakMode::FadingAIMP:
                                gv.DecaySpeed = Acceleration * (1. + (int) (gv.MaxPeakRender < 0.5));

                                gv.MaxPeakRender -= gv.DecaySpeed;

                                gv.Opacity -= gv.DecaySpeed;

                                if (gv.Opacity <= 0.)
                                    gv.MaxPeakRender = gv.PeakRender;
                                break;
                        }
                    }
                }
            }
            break;
        }

        case VisualizationType::LevelMeter:
        {
            if (isStopped)
            {
                const double Delta = 0.005;

                if (_Balance > 0.5)
                    _Balance = std::clamp(_Balance - Delta, 0.5, 1.0);
                else
                if (_Balance < 0.5)
                    _Balance = std::clamp(_Balance + Delta, 0.0, 0.5);

                if (_Phase > 0.5)
                    _Phase = std::clamp(_Phase - Delta, 0.5, 1.0);
                else
                if (_Phase < 0.5)
                    _Phase = std::clamp(_Phase + Delta, 0.0, 0.5);
            }
            break;
        }

        case VisualizationType::RadialBars:
            break;
    }
}

#pragma region Spectrum

#pragma region Frequencies

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void Analysis::GenerateLinearFrequencyBands()
{
    const double MinScale = ScaleF(_State->_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_State->_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    double i = 0.;

    for (FrequencyBand & fb: _FrequencyBands)
    {
        fb.Lo  = DeScaleF(Map(i - Bandwidth, 0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Ctr = DeScaleF(Map(i,             0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Hi  = DeScaleF(Map(i + Bandwidth, 0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%.2fHz", fb.Ctr);

        fb.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void Analysis::GenerateOctaveFrequencyBands()
{
    const double Root24 = ::exp2(1. / 24.);

    const double Pitch = (_State->_Pitch > 0.) ? ::round((::log2(_State->_Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = _State->_Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NoteGroup = 24. / _State->_BandsPerOctave;

    const double LoNote = ::round(_State->_MinNote * 2. / NoteGroup);
    const double HiNote = ::round(_State->_MaxNote * 2. / NoteGroup);

    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, (i - Bandwidth) * NoteGroup + _State->_Transpose),
            C0 * ::pow(Root24,  i              * NoteGroup + _State->_Transpose),
            C0 * ::pow(Root24, (i + Bandwidth) * NoteGroup + _State->_Transpose),
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
void Analysis::GenerateAveePlayerFrequencyBands()
{
    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    double i = 0.;

    for (FrequencyBand & fb : _FrequencyBands)
    {
        fb.Lo  = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i - Bandwidth, _State->_BandCount - 1, _State->_SkewFactor);
        fb.Ctr = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i,             _State->_BandCount - 1, _State->_SkewFactor);
        fb.Hi  = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i + Bandwidth, _State->_BandCount - 1, _State->_SkewFactor);

        fb.HasDarkBackground = true;
        ::swprintf_s(fb.Label, _countof(fb.Label), L"%.2fHz", fb.Ctr);

        ++i;
    }
}

#pragma endregion

/// <summary>
/// Gets an analyzer and its supporting objects, if any.
/// </summary>
void Analysis::GetAnalyzer(const audio_chunk & chunk) noexcept
{
    if (_WindowFunction == nullptr)
        _WindowFunction = WindowFunction::Create(_State->_WindowFunction, _State->_WindowParameter, _State->_WindowSkew, _State->_Truncate);

    if (_BrownPucketteKernel == nullptr)
        _BrownPucketteKernel = WindowFunction::Create(_State->_KernelShape, _State->_KernelShapeParameter, _State->_KernelAsymmetry, _State->_Truncate);

    if ((_FFTAnalyzer == nullptr) && (_State->_Transform == Transform::FFT))
    {
        _FFTAnalyzer = new FFTAnalyzer(_State, _SampleRate, chunk.get_channel_count(), chunk.get_channel_config(), *_WindowFunction, *_BrownPucketteKernel, _State->_BinCount);
    }

    if ((_CQTAnalyzer == nullptr) && (_State->_Transform == Transform::CQT))
    {
        _CQTAnalyzer = new CQTAnalyzer(_State, _SampleRate, chunk.get_channel_count(), chunk.get_channel_config(), *_WindowFunction);
    }

    if ((_SWIFTAnalyzer == nullptr) && (_State->_Transform == Transform::SWIFT))
    {
        _SWIFTAnalyzer = new SWIFTAnalyzer(_State, _SampleRate, chunk.get_channel_count(), chunk.get_channel_config());

        _SWIFTAnalyzer->Initialize(_FrequencyBands);
    }

    if ((_AnalogStyleAnalyzer == nullptr) && (_State->_Transform == Transform::AnalogStyle))
    {
        _AnalogStyleAnalyzer = new AnalogStyleAnalyzer(_State, _SampleRate, chunk.get_channel_count(), chunk.get_channel_config(), *_WindowFunction);

        _AnalogStyleAnalyzer->Initialize(_FrequencyBands);
    }
}

#pragma region Acoustic Weighting

/// <summary>
/// Applies acoustic weighting to the spectrum.
/// </summary>
void Analysis::ApplyAcousticWeighting()
{
    const double Offset = ((_State->_SlopeFunctionOffset * (double) _SampleRate) / (double) _State->_BinCount);

    for (FrequencyBand & fb : _FrequencyBands)
        fb.NewValue *= GetWeight(fb.Ctr + Offset);
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

#pragma region Normalization

/// <summary>
/// Normalizes the amplitude.
/// </summary>
void Analysis::Normalize() noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = std::clamp(_GraphSettings->ScaleA(fb.NewValue), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies average smoothing.
/// </summary>
void Analysis::NormalizeWithAverageSmoothing(double factor) noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = std::clamp((fb.CurValue * factor) + (::isfinite(fb.NewValue) ? _GraphSettings->ScaleA(fb.NewValue) * (1.0 - factor) : 0.0), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies peak smoothing.
/// </summary>
void Analysis::NormalizeWithPeakSmoothing(double factor) noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = std::clamp(std::max(fb.CurValue * factor, ::isfinite(fb.NewValue) ? _GraphSettings->ScaleA(fb.NewValue) : 0.0), 0.0, 1.0);
}

#pragma endregion

#pragma endregion

#pragma region Peak Meter

/// <summary>
/// Gets the values for the peak and the level meter.
/// </summary>
void Analysis::GetGaugeValues(const audio_chunk & chunk) noexcept
{
    const audio_sample * Samples = chunk.get_data();
    const size_t SampleCount = chunk.get_sample_count();

    if ((Samples == nullptr) || (SampleCount == 0))
        return;

    uint32_t ChannelMask = chunk.get_channel_config() & _GraphSettings->_Channels;

    if (ChannelMask == 0)
        return; // None of the selected channels are present in this chunk.

    InitializeGauges(ChannelMask);

    audio_sample BalanceSamples[2] = { };

    const uint32_t ChannelPairs[] =
    {
        (uint32_t) Channel::FrontLeft       | (uint32_t) Channel::FrontRight,
        (uint32_t) Channel::BackLeft        | (uint32_t) Channel::BackRight,

        (uint32_t) Channel::FrontCenterLeft | (uint32_t) Channel::FrontCenterRight,
        (uint32_t) Channel::SideLeft        | (uint32_t) Channel::SideRight,

        (uint32_t) Channel::TopFrontLeft    | (uint32_t) Channel::TopFrontRight,
        (uint32_t) Channel::TopBackLeft     | (uint32_t) Channel::TopBackRight,
    };

    const audio_sample * EndOfChunk = Samples + SampleCount;

    for (const audio_sample * Sample = Samples; Sample < EndOfChunk; Sample += chunk.get_channel_count())
    {
        const audio_sample * s = Sample;

        size_t i = 0;
        size_t j = 0;

        uint32_t ChunkChannels = chunk.get_channel_config();
        uint32_t SelectedChannels = _GraphSettings->_Channels;
        uint32_t BalanceChannels = ChannelPairs[(size_t) _State->_ChannelPair];

        while (ChunkChannels != 0)
        {
            if (ChunkChannels & 1)
            {
                if ((SelectedChannels & 1) && (i < _GaugeValues.size()))
                {
                    auto gv = &_GaugeValues[i++];

                    const double Value = std::abs((double) *s);

                    gv->Peak = std::max(Value, gv->Peak);
                    gv->RMSTotal += Value * Value;
                }

                if ((BalanceChannels & 1) && (j < _countof(BalanceSamples)))
                    BalanceSamples[j++] = *s;

                s++;
            }

            ChunkChannels    >>= 1;
            SelectedChannels >>= 1;
            BalanceChannels  >>= 1;
        }

        _Left  += BalanceSamples[0] * BalanceSamples[0];
        _Right += BalanceSamples[1] * BalanceSamples[1];

        const double Mid  = (BalanceSamples[0] + BalanceSamples[1]) / 2.;
        const double Side = (BalanceSamples[0] - BalanceSamples[1]) / 2.;

        _Mid  += Mid  * Mid;
        _Side += Side * Side;
    }

    const size_t SamplesPerChannel = SampleCount / chunk.get_channel_count();
    const double ChunkDuration = chunk.get_duration();

    _RMSSampleCount += SamplesPerChannel;
    _RMSTimeElapsed += ChunkDuration;

    // Normalize and smooth the values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
    for (auto & gv : _GaugeValues)
    {
        gv.Peak       = ToDecibel(gv.Peak);
        gv.PeakRender = SmoothValue(NormalizeValue(gv.Peak), gv.PeakRender);
    }

    // Determine the window-dependent values.
    if (_RMSTimeElapsed > _State->_RMSWindow)
    {
        for (auto & gv : _GaugeValues)
        {
        // https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
            gv.RMS       = ToDecibel(std::sqrt(gv.RMSTotal / (double) _RMSSampleCount)) + (_State->_RMSPlus3 ? dBCorrection : 0.);
            gv.RMSRender = SmoothValue(NormalizeValue(gv.RMS), gv.RMSRender);

            // Reset the RMS window values.
            gv.Reset();
        }

        _Left  = std::sqrt(_Left  / (double) _RMSSampleCount);
        _Right = std::sqrt(_Right / (double) _RMSSampleCount);

        _Mid   = std::sqrt(_Mid   / (double) _RMSSampleCount);
        _Side  = std::sqrt(_Side  / (double) _RMSSampleCount);

        if (!std::isfinite(_Balance))
            _Balance = 0.5;

        if (!std::isfinite(_Phase))
            _Phase = 0.5;

        _Balance = SmoothValue(NormalizeLRMS((_Right - _Left) / std::max(_Left, _Right)), _Balance);
        _Phase   = SmoothValue(NormalizeLRMS((_Mid   - _Side) / std::max(_Mid, _Side)), _Phase);

        // Reset all window-dependent values.
        _RMSTimeElapsed = 0.;
        _RMSSampleCount = 0;

        _Left  = 0.;
        _Right = 0.;

        _Mid   = 0.;
        _Side  = 0.;
    }
}

/// <summary>
/// Initializes the gauges before processing an audio chunk.
/// </summary>
void Analysis::InitializeGauges(uint32_t channelMask) noexcept
{
    if (_CurrentChannelMask != channelMask)
    {
        // The chunk configuration has changed. Recreate the gauges.
        static const WCHAR * ChannelNames[] =
        {
            L"FL", L"FR", L"FC",
            L"LFE",
            L"BL", L"BR", 
            L"FCL", L"FCR",
            L"BC", L"SL", L"SR", L"TC",
            L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
        };

        size_t i = 0;

        _GaugeValues.clear();

        for (uint32_t SelectedChannels = channelMask; (SelectedChannels != 0) && (i < _countof(ChannelNames)); SelectedChannels >>= 1, ++i)
        {
            if (SelectedChannels & 1)
                _GaugeValues.push_back({ ChannelNames[i], -std::numeric_limits<double>::infinity(), _State->_HoldTime });
        }

        _CurrentChannelMask = channelMask;
    }
    else
    {
        // Reset only the peak level of each gauge.
        for (auto & gv : _GaugeValues)
            gv.Peak = -std::numeric_limits<double>::infinity();
    }
}

#pragma endregion
