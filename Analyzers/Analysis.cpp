
/** $VER: Analysis.cpp (2025.10.15) P. Stuer **/

#include "pch.h"

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
void analysis_t::Initialize(const state_t * state, const graph_settings_t * settings) noexcept
{
    _State = state;
    _GraphSettings = settings;

    switch (_State->_FrequencyDistribution)
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
void analysis_t::Process(const audio_chunk & chunk) noexcept
{
    if ((_SampleRate != chunk.get_sample_rate()) || (_ChannelCount != chunk.get_channel_count()) || (_ChannelConfig != chunk.get_channel_config()))
        Reset();

    _SampleRate       = chunk.get_sample_rate();
    _NyquistFrequency = (double) _SampleRate / 2.;
    _ChannelCount     = chunk.get_channel_count();
    _ChannelConfig    = chunk.get_channel_config();

    _ChannelMask      = _ChannelConfig & _GraphSettings->_SelectedChannels;

    if (_ChannelMask == 0)
        return; // None of the selected channels are present in this chunk.

    switch (_State->_VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectogram:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
        {
            ProcessSpectrum(chunk);
            break;
        }
  
        case VisualizationType::PeakMeter:
        case VisualizationType::LevelMeter:
        {
            ProcessMeters(chunk);
            break;
        }

        case VisualizationType::Oscilloscope:
        {
            ProcessOscilloscope(chunk);
            break;
        }

        case VisualizationType::Tester:
        {
            break;
        }
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void analysis_t::Reset() noexcept
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

        InitializeGauges((uint32_t) Channels::ConfigStereo);
    }

    ResetRMSDependentValues();

    _Balance = 0.5;
    _Phase   = 0.5;
}

/// <summary>
/// Resets the RMS window dependent values.
/// </summary>
void analysis_t::ResetRMSDependentValues() noexcept
{
    _RMSTimeElapsed = 0.;
    _RMSFrameCount = 0;

    _Left  = 0.;
    _Right = 0.;

    _Mid   = 0.;
    _Side  = 0.;
}


/// <summary>
/// Updates the peak values.
/// </summary>
void analysis_t::UpdatePeakValues(bool isStopped) noexcept
{
    const double Acceleration = _State->_Acceleration / 256.;

    switch (_State->_VisualizationType)
    {
        default:
            break;

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectogram:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
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

        case VisualizationType::Oscilloscope:
        {
            _Chunk.reset();
            break;
        }

        case VisualizationType::Tester:
        {
            break;
        }
    }
}

#pragma region Spectrum

void analysis_t::ProcessSpectrum(const audio_chunk & chunk) noexcept
{
    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

    if ((Frames == nullptr) || (FrameCount == 0))
        return;

    GetAnalyzer(chunk);

    switch (_State->_Transform)
    {
        case Transform::FFT:
        {
            _FFTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphSettings->_SelectedChannels, _FrequencyBands);
            break;
        }

        case Transform::CQT:
        {
            _CQTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphSettings->_SelectedChannels, _FrequencyBands);
            break;
        }

        case Transform::SWIFT:
        {
            _SWIFTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphSettings->_SelectedChannels, _FrequencyBands);
            break;
        }

        case Transform::AnalogStyle:
        {
            _AnalogStyleAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphSettings->_SelectedChannels, _FrequencyBands);
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

    // From here on frequency_band_t::CurValue is guaranteed to be in the range 0.0 .. 1.0.
}

#pragma region Frequencies

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void analysis_t::GenerateLinearFrequencyBands()
{
    const double MinScale = ScaleFrequency(_State->_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleFrequency(_State->_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    double i = 0.;

    for (frequency_band_t & fb: _FrequencyBands)
    {
        fb.Lo  = DeScaleF(msc::Map(i - Bandwidth, 0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Center = DeScaleF(msc::Map(i,             0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Hi  = DeScaleF(msc::Map(i + Bandwidth, 0., (double)(_State->_BandCount - 1), MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);

        ::swprintf_s(fb.Label, _countof(fb.Label), L"%.2fHz", fb.Center);

        fb.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Returns the MIDI note nearest to the specified frequency.
/// </summary>
static int FrequencyToNote(double frequency) noexcept
{
    const int A4 = 69;

    return A4 + (int) ::round(12. * ::log2(frequency / 440.));
}

/// <summary>
/// Returns the frequency of the specified MIDI note.
/// </summary>
static double NoteToFrequency(int note) noexcept
{
    const int A4 = 69;

    return 440. * ::pow(2., (note - A4) / 12.);
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void analysis_t::GenerateOctaveFrequencyBands()
{
    const double Root24 = ::exp2(1. / 24.); // 24 quarter tones (https://en.wikipedia.org/wiki/Quarter_tone)

    const double TuningNote  = (_State->_TuningPitch > 0.) ? ::round(12.* (::log2(_State->_TuningPitch) - 4.)) * 2. : 0.;   // Nearest MIDI note of the tuning frequency.
    const double C0Frequency =  _State->_TuningPitch * ::pow(Root24, -TuningNote);                                          // Frequency of C0 tuned with the specified frequency (~16.35 Hz)

    const double NoteGroup = 24. / _State->_BandsPerOctave;

    const double LoIndex = ::round(_State->_MinNote * 2. / NoteGroup);
    const double HiIndex = ::round(_State->_MaxNote * 2. / NoteGroup);

    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.clear();

    static const WCHAR * NoteNames[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoIndex; i <= HiIndex; ++i)
    {
        frequency_band_t fb = 
        {
            C0Frequency * ::pow(Root24, (i - Bandwidth) * NoteGroup + _State->_Transpose),
            C0Frequency * ::pow(Root24,  i              * NoteGroup + _State->_Transpose),
            C0Frequency * ::pow(Root24, (i + Bandwidth) * NoteGroup + _State->_Transpose),
        };

        double f = NoteToFrequency(FrequencyToNote(fb.Center));

        // Pre-calculate the tooltip text and the band background color.
        {
            const uint32_t Note = (uint32_t) (i * (NoteGroup / 2.));

            const uint32_t n      = Note % (uint32_t) _countof(NoteNames);
            const uint32_t Octave = Note / (uint32_t) _countof(NoteNames);

            if (msc::InRange(f, fb.Lo, fb.Hi))
                ::swprintf_s(fb.Label, _countof(fb.Label), L"%s%d\n%.2fHz", NoteNames[n], Octave, fb.Center);
            else
                ::swprintf_s(fb.Label, _countof(fb.Label), L"%.2fHz", fb.Center);

            fb.HasDarkBackground = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        }

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Generates frequency bands like AveePlayer.
/// </summary>
void analysis_t::GenerateAveePlayerFrequencyBands()
{
    const double Bandwidth = (((_State->_Transform == Transform::FFT) && (_State->_MappingMethod == Mapping::TriangularFilterBank)) || (_State->_Transform == Transform::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    const size_t n = _State->_BandCount - 1;

    double i = 0.;

    for (frequency_band_t & fb : _FrequencyBands)
    {
        fb.Lo  = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i - Bandwidth, n, _State->_SkewFactor);
        fb.Center = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i,             n, _State->_SkewFactor);
        fb.Hi  = LogSpace(_State->_LoFrequency, _State->_HiFrequency, i + Bandwidth, n, _State->_SkewFactor);

        fb.HasDarkBackground = true;
        ::swprintf_s(fb.Label, _countof(fb.Label), L"%.2fHz", fb.Center);

        ++i;
    }
}

#pragma endregion

/// <summary>
/// Gets an analyzer and its supporting objects, if any.
/// </summary>
void analysis_t::GetAnalyzer(const audio_chunk & chunk) noexcept
{
    if (_WindowFunction == nullptr)
        _WindowFunction = window_function_t::Create(_State->_WindowFunction, _State->_WindowParameter, _State->_WindowSkew, _State->_Truncate);

    switch (_State->_Transform)
    {
        case Transform::FFT:
        {
            if (_FFTAnalyzer == nullptr)
            {       
                if (_BrownPucketteKernel == nullptr)
                    _BrownPucketteKernel = window_function_t::Create(_State->_KernelShape, _State->_KernelShapeParameter, _State->_KernelAsymmetry, _State->_Truncate);

                _FFTAnalyzer = new fft_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, *_WindowFunction, *_BrownPucketteKernel, _State->_BinCount);
            }
            break;
        }

        case Transform::CQT:
        {
            if (_CQTAnalyzer == nullptr)
                _CQTAnalyzer = new cqt_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, *_WindowFunction);
            break;
        }

        case Transform::SWIFT:
        {
            if (_SWIFTAnalyzer == nullptr)
            {
                _SWIFTAnalyzer = new swift_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig);

                _SWIFTAnalyzer->Initialize(_FrequencyBands);
            }
            break;
        }

        case Transform::AnalogStyle:
        {
            if (_AnalogStyleAnalyzer == nullptr)
            {
                _AnalogStyleAnalyzer = new analog_style_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, *_WindowFunction);

                _AnalogStyleAnalyzer->Initialize(_FrequencyBands);
            }
            break;
        }
    }
}

#pragma region Acoustic Weighting

/// <summary>
/// Applies acoustic weighting to the spectrum.
/// </summary>
void analysis_t::ApplyAcousticWeighting()
{
    const double Offset = ((_State->_SlopeFunctionOffset * (double) _SampleRate) / (double) _State->_BinCount);

    for (frequency_band_t & fb : _FrequencyBands)
        fb.NewValue *= GetWeight(fb.Center + Offset);
}

/// <summary>
/// Gets the total weight.
/// </summary>
double analysis_t::GetWeight(double x) const noexcept
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
void analysis_t::Normalize() noexcept
{
    for (frequency_band_t & fb : _FrequencyBands)
        fb.CurValue = std::clamp(_GraphSettings->ScaleAmplitude(fb.NewValue), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies average smoothing.
/// </summary>
void analysis_t::NormalizeWithAverageSmoothing(double factor) noexcept
{
    for (frequency_band_t & fb : _FrequencyBands)
        fb.CurValue = std::clamp((fb.CurValue * factor) + (::isfinite(fb.NewValue) ? _GraphSettings->ScaleAmplitude(fb.NewValue) * (1.0 - factor) : 0.0), 0.0, 1.0);
}

/// <summary>
/// Normalizes the amplitude and applies peak smoothing.
/// </summary>
void analysis_t::NormalizeWithPeakSmoothing(double factor) noexcept
{
    for (frequency_band_t & fb : _FrequencyBands)
        fb.CurValue = std::clamp(std::max(fb.CurValue * factor, ::isfinite(fb.NewValue) ? _GraphSettings->ScaleAmplitude(fb.NewValue) : 0.0), 0.0, 1.0);
}

#pragma endregion

#pragma endregion

#pragma region Peak Meter

/// <summary>
/// Process the chunk data for the peak and the level meter.
/// </summary>
void analysis_t::ProcessMeters(const audio_chunk & chunk) noexcept
{
    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

    if ((Frames == nullptr) || (FrameCount == 0))
        return;

    InitializeGauges(_ChannelMask);

    audio_sample BalanceSamples[2] = { };

    const audio_sample * EndOfChunk = Frames + (FrameCount * _ChannelCount);

    for (const audio_sample * Frame = Frames; Frame < EndOfChunk; Frame += _ChannelCount)
    {
        const audio_sample * Sample = Frame;    // First sample of the current frame.

        size_t i = 0;
        size_t j = 0;

        uint32_t ChunkChannels    = chunk.get_channel_config();                     // Mask containing the channels in the audio chunk.
        uint32_t SelectedChannels = _GraphSettings->_SelectedChannels;              // Mask containing the channels selected by the user for the level measuring.
        uint32_t BalanceChannels  = ChannelPairs[(size_t) _State->_ChannelPair];    // Mask containing the channels selected by the user for the balance measuring.

        while ((ChunkChannels & SelectedChannels & BalanceChannels) != 0)
        {
            if (ChunkChannels & 1)
            {
                if ((SelectedChannels & 1) && (i < _GaugeValues.size()))
                {
                    auto gv = &_GaugeValues[i++];

                    const double Value = std::abs((double) *Sample);

                    gv->Peak = std::max(Value, gv->Peak);
                    gv->RMSTotal += Value * Value;
                }

                if ((BalanceChannels & 1) && (j < _countof(BalanceSamples)))
                    BalanceSamples[j++] = *Sample;

                Sample++;
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

    _RMSFrameCount  += FrameCount;
    _RMSTimeElapsed += chunk.get_duration();

    // Normalize and smooth the peak values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
    for (auto & gv : _GaugeValues)
    {
        gv.Peak       = ToDecibel(gv.Peak);
        gv.PeakRender = SmoothValue(NormalizeValue(gv.Peak), gv.PeakRender);
    }

    // Has the RMS window elapsed yet?
    if (_RMSTimeElapsed > _State->_RMSWindow)
    {
        for (auto & gv : _GaugeValues)
        {
            // https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
            gv.RMS       = ToDecibel(std::sqrt(gv.RMSTotal / (double) _RMSFrameCount)) + (_State->_RMSPlus3 ? dBCorrection : 0.);
            gv.RMSRender = SmoothValue(NormalizeValue(gv.RMS), gv.RMSRender);

            // Reset the RMS window-dependent values.
            gv.RMSTotal = 0.;
        }

        // Calculate the phase and balance.
        {
            {
                _Left  = std::sqrt(_Left  / (double) _RMSFrameCount);
                _Right = std::sqrt(_Right / (double) _RMSFrameCount);

                if (!std::isfinite(_Balance))
                    _Balance = 0.5;

                _Balance = SmoothValue(NormalizeLLevelValue((_Right - _Left) / std::max(_Left, _Right)), _Balance);
            }

            {
                _Mid   = std::sqrt(_Mid   / (double) _RMSFrameCount);
                _Side  = std::sqrt(_Side  / (double) _RMSFrameCount);

                if (!std::isfinite(_Phase))
                    _Phase = 0.5;

                _Phase = SmoothValue(NormalizeLLevelValue((_Mid - _Side) / std::max(_Mid, _Side)), _Phase);
            }
        }

        ResetRMSDependentValues();
    }
}

/// <summary>
/// Initializes the gauges before processing an audio chunk.
/// </summary>
void analysis_t::InitializeGauges(uint32_t channelMask) noexcept
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
                _GaugeValues.push_back({ ChannelNames[i], _State->_HoldTime });
        }

        _CurrentChannelMask = channelMask;
    }
    else
    {
        // Reset only the peak level of each gauge to -∞.
        for (auto & gv : _GaugeValues)
            gv.Peak = -std::numeric_limits<double>::infinity();
    }
}

#pragma endregion

#pragma region Oscilloscope

/// <summary>
/// Process the chunk data for the oscilloscope.
/// </summary>
void analysis_t::ProcessOscilloscope(const audio_chunk & chunk) noexcept
{
    _Chunk.copy(chunk);
}

#pragma endregion

const uint32_t analysis_t::ChannelPairs[6] =
{
    (uint32_t) Channels::FrontLeft       | (uint32_t) Channels::FrontRight,
    (uint32_t) Channels::BackLeft        | (uint32_t) Channels::BackRight,

    (uint32_t) Channels::FrontCenterLeft | (uint32_t) Channels::FrontCenterRight,
    (uint32_t) Channels::SideLeft        | (uint32_t) Channels::SideRight,

    (uint32_t) Channels::TopFrontLeft    | (uint32_t) Channels::TopFrontRight,
    (uint32_t) Channels::TopBackLeft     | (uint32_t) Channels::TopBackRight,
};
