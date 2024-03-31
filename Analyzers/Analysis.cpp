
/** $VER: Analysis.cpp (2024.03.31) P. Stuer **/

#include "Analysis.h"

#include "Support.h"

#pragma hdrstop

inline double GetFrequencyTilt(double x, double amount, double offset) noexcept;
inline double Equalize(double x, double amount, double depth, double offset) noexcept;
inline double GetAcousticWeight(double x, WeightingType weightingType, double weightAmount) noexcept;

/// <summary>
/// Initializes this instance.
/// </summary>
void Analysis::Initialize(const State * threadState, const GraphSettings * settings) noexcept
{
    _ThreadState = threadState;
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

    _MeterValues.clear();
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

    GetMeterValues(chunk);

    GetAnalyzer(chunk);

    switch (_ThreadState->_Transform)
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
    if (_ThreadState->_WeightingType != WeightingType::None)
        ApplyAcousticWeighting();

    // Smooth the spectrum.
    switch (_ThreadState->_SmoothingMethod)
    {
        default:

        case SmoothingMethod::None:
        {
            Normalize();
            break;
        }

        case SmoothingMethod::Average:
        {
            NormalizeWithAverageSmoothing(_ThreadState->_SmoothingFactor);
            break;
        }

        case SmoothingMethod::Peak:
        {
            NormalizeWithPeakSmoothing(_ThreadState->_SmoothingFactor);
            break;
        }
    }

    // From here one CurValue is guaranteed in the range 0.0 .. 1.0
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
}

#pragma region Frequencies

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void Analysis::GenerateLinearFrequencyBands()
{
    const double MinScale = ScaleF(_ThreadState->_LoFrequency, _ThreadState->_ScalingFunction, _ThreadState->_SkewFactor);
    const double MaxScale = ScaleF(_ThreadState->_HiFrequency, _ThreadState->_ScalingFunction, _ThreadState->_SkewFactor);

    const double Bandwidth = (((_ThreadState->_Transform == Transform::FFT) && (_ThreadState->_MappingMethod == Mapping::TriangularFilterBank)) || (_ThreadState->_Transform == Transform::CQT)) ? _ThreadState->_Bandwidth : 0.5;

    _FrequencyBands.resize(_ThreadState->_BandCount);

    double i = 0.;

    for (FrequencyBand & fb: _FrequencyBands)
    {
        fb.Lo  = DeScaleF(Map(i - Bandwidth, 0., (double)(_ThreadState->_BandCount - 1), MinScale, MaxScale), _ThreadState->_ScalingFunction, _ThreadState->_SkewFactor);
        fb.Ctr = DeScaleF(Map(i,             0., (double)(_ThreadState->_BandCount - 1), MinScale, MaxScale), _ThreadState->_ScalingFunction, _ThreadState->_SkewFactor);
        fb.Hi  = DeScaleF(Map(i + Bandwidth, 0., (double)(_ThreadState->_BandCount - 1), MinScale, MaxScale), _ThreadState->_ScalingFunction, _ThreadState->_SkewFactor);

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

    const double Pitch = (_ThreadState->_Pitch > 0.) ? ::round((::log2(_ThreadState->_Pitch) - 4.) * 12.) * 2. : 0.;
    const double C0 = _ThreadState->_Pitch * ::pow(Root24, -Pitch); // ~16.35 Hz

    const double NoteGroup = 24. / _ThreadState->_BandsPerOctave;

    const double LoNote = ::round(_ThreadState->_MinNote * 2. / NoteGroup);
    const double HiNote = ::round(_ThreadState->_MaxNote * 2. / NoteGroup);

    const double Bandwidth = (((_ThreadState->_Transform == Transform::FFT) && (_ThreadState->_MappingMethod == Mapping::TriangularFilterBank)) || (_ThreadState->_Transform == Transform::CQT)) ? _ThreadState->_Bandwidth : 0.5;

    _FrequencyBands.clear();

    static const WCHAR * NoteName[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoNote; i <= HiNote; ++i)
    {
        FrequencyBand fb = 
        {
            C0 * ::pow(Root24, (i - Bandwidth) * NoteGroup + _ThreadState->_Transpose),
            C0 * ::pow(Root24,  i              * NoteGroup + _ThreadState->_Transpose),
            C0 * ::pow(Root24, (i + Bandwidth) * NoteGroup + _ThreadState->_Transpose),
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
    const double Bandwidth = (((_ThreadState->_Transform == Transform::FFT) && (_ThreadState->_MappingMethod == Mapping::TriangularFilterBank)) || (_ThreadState->_Transform == Transform::CQT)) ? _ThreadState->_Bandwidth : 0.5;

    _FrequencyBands.resize(_ThreadState->_BandCount);

    double i = 0.;

    for (FrequencyBand & fb : _FrequencyBands)
    {
        fb.Lo  = LogSpace(_ThreadState->_LoFrequency, _ThreadState->_HiFrequency, i - Bandwidth, _ThreadState->_BandCount - 1, _ThreadState->_SkewFactor);
        fb.Ctr = LogSpace(_ThreadState->_LoFrequency, _ThreadState->_HiFrequency, i,             _ThreadState->_BandCount - 1, _ThreadState->_SkewFactor);
        fb.Hi  = LogSpace(_ThreadState->_LoFrequency, _ThreadState->_HiFrequency, i + Bandwidth, _ThreadState->_BandCount - 1, _ThreadState->_SkewFactor);

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
        _WindowFunction = WindowFunction::Create(_ThreadState->_WindowFunction, _ThreadState->_WindowParameter, _ThreadState->_WindowSkew, _ThreadState->_Truncate);

    if (_BrownPucketteKernel == nullptr)
        _BrownPucketteKernel = WindowFunction::Create(_ThreadState->_KernelShape, _ThreadState->_KernelShapeParameter, _ThreadState->_KernelAsymmetry, _ThreadState->_Truncate);

    uint32_t ChannelCount = chunk.get_channel_count();
    uint32_t ChannelSetup = chunk.get_channel_config();

    if ((_FFTAnalyzer == nullptr) && (_ThreadState->_Transform == Transform::FFT))
    {
        _FFTAnalyzer = new FFTAnalyzer(_ThreadState, _SampleRate, ChannelCount, ChannelSetup, *_WindowFunction, *_BrownPucketteKernel, _ThreadState->_BinCount);
    }

    if ((_CQTAnalyzer == nullptr) && (_ThreadState->_Transform == Transform::CQT))
    {
        _CQTAnalyzer = new CQTAnalyzer(_ThreadState, _SampleRate, ChannelCount, ChannelSetup, *_WindowFunction);
    }

    if ((_SWIFTAnalyzer == nullptr) && (_ThreadState->_Transform == Transform::SWIFT))
    {
        _SWIFTAnalyzer = new SWIFTAnalyzer(_ThreadState, _SampleRate, ChannelCount, ChannelSetup);

        _SWIFTAnalyzer->Initialize(_FrequencyBands);
    }

    if ((_AnalogStyleAnalyzer == nullptr) && (_ThreadState->_Transform == Transform::AnalogStyle))
    {
        _AnalogStyleAnalyzer = new AnalogStyleAnalyzer(_ThreadState, _SampleRate, ChannelCount, ChannelSetup, *_WindowFunction);

        _AnalogStyleAnalyzer->Initialize(_FrequencyBands);
    }
}

#pragma region Acoustic Weighting

/// <summary>
/// Applies acoustic weighting to the spectrum.
/// </summary>
void Analysis::ApplyAcousticWeighting()
{
    const double Offset = ((_ThreadState->_SlopeFunctionOffset * (double) _SampleRate) / (double) _ThreadState->_BinCount);

    for (FrequencyBand & fb : _FrequencyBands)
        fb.NewValue *= GetWeight(fb.Ctr + Offset);
}

/// <summary>
/// Gets the total weight.
/// </summary>
double Analysis::GetWeight(double x) const noexcept
{
    const double a = GetFrequencyTilt(x, _ThreadState->_Slope, _ThreadState->_SlopeOffset);
    const double b = Equalize(x, _ThreadState->_EqualizeAmount, _ThreadState->_EqualizeDepth, _ThreadState->_EqualizeOffset);
    const double c = GetAcousticWeight(x, _ThreadState->_WeightingType, _ThreadState->_WeightingAmount);

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
        fb.CurValue = Clamp(_GraphSettings->ScaleA(fb.NewValue), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies average smoothing.
/// </summary>
void Analysis::NormalizeWithAverageSmoothing(double factor) noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = Clamp((fb.CurValue * factor) + (::isfinite(fb.NewValue) ? _GraphSettings->ScaleA(fb.NewValue) * (1.0 - factor) : 0.0), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies peak smoothing.
/// </summary>
void Analysis::NormalizeWithPeakSmoothing(double factor) noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
        fb.CurValue = Clamp(Max(fb.CurValue * factor, ::isfinite(fb.NewValue) ? _GraphSettings->ScaleA(fb.NewValue) : 0.0), 0.0, 1.0);
}

#pragma endregion

/// <summary>
/// Updates the value of the peak indicators.
/// </summary>
void Analysis::UpdatePeakIndicators() noexcept
{
    for (FrequencyBand & fb : _FrequencyBands)
    {
        if (fb.CurValue >= fb.Peak)
        {
            if ((_ThreadState->_PeakMode == PeakMode::AIMP) || (_ThreadState->_PeakMode == PeakMode::FadingAIMP))
                fb.HoldTime = (::isfinite(fb.HoldTime) ? fb.HoldTime : 0.) + (fb.CurValue - fb.Peak) * _ThreadState->_HoldTime;
            else
                fb.HoldTime = _ThreadState->_HoldTime;

            fb.Peak = fb.CurValue;
            fb.DecaySpeed = 0.;
            fb.Opacity = 1.;
        }
        else
        {
            if (fb.HoldTime >= 0.)
            {
                if ((_ThreadState->_PeakMode == PeakMode::AIMP) || (_ThreadState->_PeakMode == PeakMode::FadingAIMP))
                    fb.Peak += (fb.HoldTime - Max(fb.HoldTime - 1., 0.)) / _ThreadState->_HoldTime;

                fb.HoldTime -= 1.;

                if ((_ThreadState->_PeakMode == PeakMode::AIMP) || (_ThreadState->_PeakMode == PeakMode::FadingAIMP))
                    fb.HoldTime = Min(fb.HoldTime, _ThreadState->_HoldTime);
            }
            else
            {
                switch (_ThreadState->_PeakMode)
                {
                    default:

                    case PeakMode::None:
                        break;

                    case PeakMode::Classic:
                        fb.DecaySpeed = _ThreadState->_Acceleration / 256.;
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::Gravity:
                        fb.DecaySpeed += _ThreadState->_Acceleration / 256.;
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::AIMP:
                        fb.DecaySpeed = (_ThreadState->_Acceleration / 256.) * (1. + (int) (fb.Peak < 0.5));
                        fb.Peak -= fb.DecaySpeed;
                        break;

                    case PeakMode::FadeOut:
                        fb.DecaySpeed += _ThreadState->_Acceleration / 256.;
                        fb.Opacity -= fb.DecaySpeed;

                        if (fb.Opacity <= 0.)
                            fb.Peak = fb.CurValue;
                        break;

                    case PeakMode::FadingAIMP:
                        fb.DecaySpeed = (_ThreadState->_Acceleration / 256.) * (1. + (int) (fb.Peak < 0.5));
                        fb.Peak -= fb.DecaySpeed;
                        fb.Opacity -= fb.DecaySpeed;

                        if (fb.Opacity <= 0.)
                            fb.Peak = fb.CurValue;
                        break;
                }
            }

            fb.Peak = Clamp(fb.Peak, 0., 1.);
        }
    }
}

/// <summary>
/// Gets the Peak and RMS level (Root Mean Square level) values of each channel.
/// </summary>
bool Analysis::GetMeterValues(const audio_chunk & chunk) noexcept
{
    const audio_sample * Samples = chunk.get_data();
    const size_t SampleCount = chunk.get_sample_count();

    if ((Samples == nullptr) || (SampleCount == 0))
        return false;

    if (_MeterValues.size() != chunk.get_channel_count())
    {
        static const WCHAR * ChannelNames[] =
        {
            L"FL", L"FR", L"FC",
            L"LFE",
            L"BL", L"BR", 
            L"FCL", L"FCR",
            L"BC", L"SL", L"SR", L"TC",
            L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
        };

        _MeterValues.clear();

        if (chunk.get_channel_count() != 1)
        {
            size_t i = 0;

            for (unsigned ChannelConfig = chunk.get_channel_config() & _GraphSettings->_Channels; (ChannelConfig != 0) && (i < _countof(ChannelNames)); ChannelConfig >>= 1, ++i)
            {
                if (ChannelConfig & 1)
                    _MeterValues.push_back({ 0.0, 0.0, ChannelNames[i] });
            }
        }
        else
            _MeterValues.push_back({ 0.0, 0.0, ChannelNames[2] }); // Most likely only FL and FR are enabled by the user. Mono track will cause an infinite loop.
    }
    else
    {
        for (auto & mv : _MeterValues)
        {
            mv.NewPeak = 0.0;
            mv.NewRMS = 0.0;
            mv.ScaledPeak = -999.0;
            mv.ScaledRMS = -999.0;
        }
    }

    const audio_sample * EndOfChunk = Samples + SampleCount;

    for (const audio_sample * Sample = Samples; Sample < EndOfChunk; )
    {
        for (auto & mv : _MeterValues)
        {
            audio_sample Value = std::abs(*Sample);

            if (Value > mv.NewPeak)
                mv.NewPeak = Value;

            mv.NewRMS += Value * Value;

            ++Sample;
        }
    }

    // Calculate the scaled values. Keep the new value only when it's larger than the current value to reduce the 'jumpiness' of the meter.
    for (auto & mv : _MeterValues)
    {
        double ScaledPeak = ToDecibel(mv.NewPeak);

        if (ScaledPeak > mv.ScaledPeak)
            mv.ScaledPeak = ScaledPeak;

        mv.NewRMS = (audio_sample) std::sqrt(mv.NewRMS / (audio_sample) SampleCount);

        double ScaledRMS = ToDecibel(mv.NewRMS);

        if (ScaledRMS > mv.ScaledRMS)
            mv.ScaledRMS = ScaledRMS;
    }

    return true;
}
