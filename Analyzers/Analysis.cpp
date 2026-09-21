
/** $VER: Analysis.cpp (2026.09.20) P. Stuer **/

#include "pch.h"

#include "Analysis.h"
#include "Downmixer.h"
#include "FrequencyScaler.h"

#include "Support.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void analysis_t::Initialize(const state_t * state, const graph_options_t * graphOptions) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;

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

        case FrequencyDistribution::Mel:
            GenerateMelFrequencyBands();
            break;
    }

    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void analysis_t::Reset() noexcept
{
    if (_State == nullptr)
        return;

    _SampleRate    = 0;
    _ChannelCount  = 0;
    _ChannelConfig = 0;

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

    // FFT-based visualizations
    for (auto & fb : _FrequencyBands)
    {
        fb.Value = 0.;

        if (_State->_ResetPeaksOnTrackChange)
        {
            fb.PeakValue = 0.;
            fb.FallRate = 0.;
            fb.Opacity  = 1.;
        }
    }

    // Peak/RMS Meter
    {
        ResetPeakMeasurements();
        ResetRMSDependentValues();

        InitializePeakMeasurements((uint32_t) Channels::ConfigStereo);
    }

    // Balance/Correlation Meter
    {
        _Balance = 0.5;
        _Phase   = 0.5;
    }

    // Bit Meter
    {
        _BitActiveChannelMask = 0;

        InitializeBitMeasurements((uint32_t) Channels::ConfigStereo);
    }

    _Chrono.Reset();
}

/// <summary>
/// Processes an audio chunk.
/// </summary>
void analysis_t::Process(const audio_chunk & chunk) noexcept
{
    if ((_SampleRate != chunk.get_sample_rate()) || (_ChannelCount != chunk.get_channel_count()) || (_ChannelConfig != chunk.get_channel_config()))
        Reset();

    _SampleRate       = chunk.get_sample_rate();
    _ChannelCount     = chunk.get_channel_count();
    _ChannelConfig    = chunk.get_channel_config();

    _NyquistFrequency = (double) _SampleRate / 2.;
    _ChannelMask      = _ChannelConfig & _GraphOptions->_ActiveChannelMask;

    if (_ChannelMask == 0)
        return; // None of the selected channels are present in this chunk.

    switch (_State->_VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectrogram:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
        {
            SpectrumProcessing(chunk);
            break;
        }
  
        case VisualizationType::PeakMeter:
        case VisualizationType::LevelMeter:
        {
            MeterProcessing(chunk);
            break;
        }

        case VisualizationType::Oscilloscope:
        {
            OscilloscopeProcessing(chunk);
            break;
        }

        case VisualizationType::BitMeter:
        {
            BitMeterProcessing(chunk);
            break;
        }

        case VisualizationType::Goniometer:
        {
            StereoMeterProcessing(chunk);
            break;
        }

        case VisualizationType::Tester:
        {
            SpectrumProcessing(chunk);
            _Chunk.copy(chunk, true);
            break;
        }
    }
}

/// <summary>
/// Resets the peak measurements.
/// </summary>
void analysis_t::ResetPeakMeasurements() noexcept
{
    for (peak_measurement_t & m : _PeakMeasurements)
    {
        m.Peak = m.RMS = -std::numeric_limits<double>::infinity();
        m.NormalizedPeak = m.NormalizedRMS = 0.;
    }
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
    if ((_State == nullptr) || (_GraphOptions == nullptr))
        return;

    const double Elapsed = _Chrono.Elapsed();
    const double AmplitudeRange = _GraphOptions->_AmplitudeHi - _GraphOptions->_AmplitudeLo;

    const double HoldTime = _State->_HoldTime * (double) _State->_RefreshRateLimit;
    const double FallRate = (_State->_FallRate != 0) ? msc::Map(Elapsed, 0., AmplitudeRange / _State->_FallRate, 0., 1.) : 0.;

    const bool IsAIMP = (_State->_PeakMode == PeakMode::AIMP) || (_State->_PeakMode == PeakMode::FadingAIMP);

    switch (_State->_VisualizationType)
    {
        default:
            break;

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::Spectrogram:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
        {
            // Animate the spectrum peak value.
            for (auto & fb : _FrequencyBands)
            {
                if (fb.Value >= fb.PeakValue)
                {
                    if (IsAIMP)
                        fb.HoldTime += (fb.Value - fb.PeakValue) * HoldTime;
                    else
                        fb.HoldTime = HoldTime;

                    fb.PeakValue = fb.Value;
                    fb.FallRate = 0.;
                    fb.Opacity  = 1.;
                }
                else
                {
                    if (fb.HoldTime > 0.)
                    {
                        if (IsAIMP)
                            fb.PeakValue += (fb.HoldTime - std::max(fb.HoldTime - 1., 0.)) / HoldTime;

                        fb.HoldTime--;

                        if (IsAIMP)
                            fb.HoldTime = std::min(fb.HoldTime, HoldTime);
                    }
                    else
                    {
                        switch (_State->_PeakMode)
                        {
                            default:

                            case PeakMode::None:
                                break;

                            case PeakMode::Classic:
                            {
                                constexpr double FallAcceleration = 0.1;

                                fb.FallRate  = std::min(fb.FallRate + FallAcceleration, FallRate);
                                fb.PeakValue -= fb.FallRate;
                                break;
                            }

                            case PeakMode::Gravity:
                            {
                                fb.FallRate += FallRate;
                                fb.PeakValue -= fb.FallRate;
                                break;
                            }

                            case PeakMode::FadeOut:
                            {
                                fb.FallRate += FallRate;

                                fb.Opacity -= fb.FallRate;

                                if (fb.Opacity <= 0.)
                                    fb.PeakValue = fb.Value;
                                break;
                            }

                            case PeakMode::AIMP:
                            {
                                fb.FallRate  = FallRate * (1. + (int) (fb.PeakValue < 0.5));
                                fb.PeakValue -= fb.FallRate;
                                break;
                            }

                            case PeakMode::FadingAIMP:
                            {
                                fb.FallRate  = FallRate * (1. + (int) (fb.PeakValue < 0.5));
                                fb.PeakValue -= fb.FallRate;

                                fb.Opacity -= fb.FallRate;

                                if (fb.Opacity <= 0.)
                                    fb.PeakValue = fb.Value;
                                break;
                            }
                        }
                    }

                    fb.PeakValue = std::clamp(fb.PeakValue, fb.Value, 1.);
                }
            }
            break;
        }

        case VisualizationType::PeakMeter:
        {
            // Animate the smoothed peak and RMS values.
            for (auto & m : _PeakMeasurements)
            {
                if (m.NormalizedPeak >= m.MaxNormalizedPeak)
                {
                    if (IsAIMP)
                        m.HoldTime += (m.NormalizedPeak - m.MaxNormalizedPeak) * HoldTime;
                    else
                        m.HoldTime = HoldTime;

                    m.MaxNormalizedPeak = m.NormalizedPeak;
                    m.FallRate          = 0.;
                    m.Opacity           = 1.;
                }
                else
                {
                    if (m.HoldTime > 0.)
                    {
                        if (IsAIMP)
                            m.MaxNormalizedPeak += (m.HoldTime - std::max(m.HoldTime - 1., 0.)) / HoldTime;

                        m.HoldTime--;

                        if (IsAIMP)
                            m.HoldTime = std::min(m.HoldTime, HoldTime);
                    }
                    else
                    {
                        switch (_State->_PeakMode)
                        {
                            default:

                            case PeakMode::None:
                                break;

                            case PeakMode::Classic:
                            {
                                m.FallRate           = FallRate;
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::Gravity:
                            {
                                m.FallRate          += FallRate;
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::FadeOut:
                            {
                                m.FallRate += FallRate;

                                m.Opacity -= m.FallRate;

                                if (m.Opacity <= 0.)
                                    m.MaxNormalizedPeak = m.NormalizedPeak;
                                break;
                            }

                            case PeakMode::AIMP:
                            {
                                m.FallRate           = FallRate * (1. + (int) (m.MaxNormalizedPeak < 0.5));
                                m.MaxNormalizedPeak -= m.FallRate;
                                break;
                            }

                            case PeakMode::FadingAIMP:
                            {
                                m.FallRate           = FallRate * (1. + (int) (m.MaxNormalizedPeak < 0.5));
                                m.MaxNormalizedPeak -= m.FallRate;

                                m.Opacity -= m.FallRate;

                                if (m.Opacity <= 0.)
                                    m.MaxNormalizedPeak = m.NormalizedPeak;
                                break;
                            }
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
        case VisualizationType::BitMeter:
        case VisualizationType::Goniometer:
        {
            if (isStopped)
                _Chunk.reset();
            break;
        }

        case VisualizationType::Tester:
        {
            break;
        }
    }

    _Chrono.Reset();
}

#pragma region Frequencies

/// <summary>
/// Generates frequency bands using a linear distribution.
/// </summary>
void analysis_t::GenerateLinearFrequencyBands()
{
    assert(_State->_BandCount != 0);

    const double MinScale = ScaleFrequency(_State->_LoFrequency, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleFrequency(_State->_HiFrequency, _State->_ScalingFunction, _State->_SkewFactor);

    const double Bandwidth = (((_State->_TransformMethod == TransformMethod::FFT) && (_State->_MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State->_TransformMethod == TransformMethod::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    double i = 0.;

    const double MaxIndex = (double) (_State->_BandCount - 1);

    for (frequency_band_t & fb: _FrequencyBands)
    {
        const double LoIndex = std::clamp(i - Bandwidth, 0., MaxIndex);
        const double HiIndex = std::clamp(i + Bandwidth, 0., MaxIndex);

        fb.Lo  = DescaleFrequency(msc::Map(LoIndex, 0., MaxIndex, MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Mid = DescaleFrequency(msc::Map(i,       0., MaxIndex, MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);
        fb.Hi  = DescaleFrequency(msc::Map(HiIndex, 0., MaxIndex, MinScale, MaxScale), _State->_ScalingFunction, _State->_SkewFactor);

        assert(std::isfinite(fb.Lo));
        assert(std::isfinite(fb.Mid));
        assert(std::isfinite(fb.Hi));

        assert(fb.Lo <= fb.Mid && fb.Mid <= fb.Hi);

        ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _GraphOptions->_XAxisDecimals, fb.Mid);

        fb.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Returns the MIDI note nearest to the specified frequency.
/// </summary>
static inline int FrequencyToNote(double frequency) noexcept
{
    constexpr int A4 = 69;

    return A4 + (int) std::round(12. * std::log2(frequency / 440.));
}

/// <summary>
/// Returns the frequency of the specified MIDI note.
/// </summary>
static inline double NoteToFrequency(int note) noexcept
{
    constexpr int A4 = 69;

    return 440. * std::pow(2., (note - A4) / 12.);
}

/// <summary>
/// Generates frequency bands based on the frequencies of musical notes.
/// </summary>
void analysis_t::GenerateOctaveFrequencyBands()
{
    assert(_State->_TuningPitch > 0.); assert(_State->_BandsPerOctave != 0);

    const double Root24 = std::exp2(1. / 24.); // 24 quarter tones (https://en.wikipedia.org/wiki/Quarter_tone)

    constexpr double C0 = 16.35; // Hz

    const double TuningOffset = (_State->_TuningPitch > 0.) ? std::round(12.* std::log2(_State->_TuningPitch / C0)) * 2. : 0.;  // Number of quarter-tone steps between C0 and the nearest equal-tempered semitone corresponding to the tuning frequency.
    const double C0Frequency  =  _State->_TuningPitch * std::pow(Root24, -TuningOffset);                                        // Frequency of C0 tuned with the specified frequency (~16.35 Hz)

    const double NoteGroup = 24. / _State->_BandsPerOctave;

    const double LoIndex = std::round(_State->_LoNote * 2. / NoteGroup);
    const double HiIndex = std::round(_State->_HiNote * 2. / NoteGroup);

    assert(LoIndex <= HiIndex);

    const double Bandwidth = (((_State->_TransformMethod == TransformMethod::FFT) && (_State->_MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State->_TransformMethod == TransformMethod::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.reserve((size_t) (HiIndex - LoIndex + 1.));

    static constexpr const WCHAR * NoteNames[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

    for (double i = LoIndex; i <= HiIndex; ++i)
    {
        frequency_band_t fb = 
        {
            C0Frequency * std::pow(Root24, (i - Bandwidth) * NoteGroup + _State->_Transpose),
            C0Frequency * std::pow(Root24,  i              * NoteGroup + _State->_Transpose),
            C0Frequency * std::pow(Root24, (i + Bandwidth) * NoteGroup + _State->_Transpose),
        };

        assert(fb.Lo <= fb.Mid && fb.Mid <= fb.Hi);

        const double f = NoteToFrequency(FrequencyToNote(fb.Mid));

        // Pre-calculate the tooltip text and the band background color.
        {
            const uint32_t Note = (uint32_t) (i * (NoteGroup / 2.));

            const uint32_t n      = Note % (uint32_t) _countof(NoteNames);
            const uint32_t Octave = Note / (uint32_t) _countof(NoteNames);

            if (msc::InRange(f, fb.Lo, fb.Hi))
                ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%s%d\n%.*f Hz", NoteNames[n], Octave, _GraphOptions->_XAxisDecimals, fb.Mid);
            else
                ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _GraphOptions->_XAxisDecimals, fb.Mid);

            fb.HasDarkBackground = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        }

        _FrequencyBands.push_back(fb);
    }
}

/// <summary>
/// Calculates a frequency value for a given band index between minFreq and maxFreq.
/// The skew factor determines the interpolation between the logarithmic and linear scale.
/// skewFactor = 0.0: Pure logarithmic spacing
/// skewFactor = 1.0: Pure linear spacing
/// skewFactor = 0.5: 50% mix of both
/// </summary>
static inline double CalcBlendedLogLinearFrequency(double minFreq, double maxFreq, double bandIndex, double maxBandIndex, double skewFactor) noexcept
{
    assert(minFreq > 0.); assert(maxFreq > 0.); assert(bandIndex <= maxBandIndex); assert(maxBandIndex != 0); assert(0. <= skewFactor && skewFactor <= 1.);

    // Calculate the frequency on a logarithmic scale. Good for audio frequencies and human perception.
    const double f1 = minFreq * std::pow((maxFreq / minFreq), (bandIndex / maxBandIndex));

    // Calculate the frequency on a linear scale. Even numerical distance between frequencies.
    const double f2 = minFreq + ((maxFreq - minFreq) * (bandIndex / maxBandIndex));

    // Blend the two results using linear interpolation.
    return std::lerp(f1, f2, skewFactor);
}

/// <summary>
/// Generates frequency bands like AveePlayer.
/// </summary>
void analysis_t::GenerateAveePlayerFrequencyBands()
{
    assert(_State->_BandCount > 1); assert(0. <= _State->_Bandwidth && _State->_Bandwidth <= 64.);

    const double Bandwidth = (((_State->_TransformMethod == TransformMethod::FFT) && (_State->_MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State->_TransformMethod == TransformMethod::CQT)) ? _State->_Bandwidth : 0.5;

    _FrequencyBands.resize(_State->_BandCount);

    const double MaxIndex = (double) (_State->_BandCount - 1);

    double i = 0.;

    for (frequency_band_t & fb : _FrequencyBands)
    {
        const double LoIndex = std::clamp(i - Bandwidth, 0., MaxIndex);
        const double HiIndex = std::clamp(i + Bandwidth, 0., MaxIndex);

        fb.Lo  = CalcBlendedLogLinearFrequency(_State->_LoFrequency, _State->_HiFrequency, LoIndex, MaxIndex, _State->_SkewFactor);
        fb.Mid = CalcBlendedLogLinearFrequency(_State->_LoFrequency, _State->_HiFrequency, i,       MaxIndex, _State->_SkewFactor);
        fb.Hi  = CalcBlendedLogLinearFrequency(_State->_LoFrequency, _State->_HiFrequency, HiIndex, MaxIndex, _State->_SkewFactor);

        assert(fb.Lo <= fb.Mid && fb.Mid <= fb.Hi);

        ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _GraphOptions->_XAxisDecimals, fb.Mid);

        fb.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Converts a frequency in Hz to the Mel scale.
/// </summary>
static inline double HzToMel(const double frequency) noexcept
{
    return 2595. * std::log10(1. + frequency / 700.);
}

/// <summary>
/// Converts a value on the Mel scale to Hz.
/// </summary>
static inline double MelToHz(const double mel) noexcept
{
    return 700. * (std::pow(10., mel / 2595.) - 1.);
}

/// <summary>
/// Generates triangular frequency bands spaced uniformly on the Mel scale.
/// </summary>
/// <ref>https://deepwiki.com/dspavankumar/compute-mfcc/2.4.2-mel-filterbank-construction</ref>
void analysis_t::GenerateMelFrequencyBands()
{
    // Generate the frequencies for each of the Mel points.
    std::vector<double> Frequencies(_State->_MelBandCount + 2);
    {
        assert(_State->_LoFrequency < _State->_HiFrequency);

        const double LoMel   = HzToMel(_State->_LoFrequency);
        const double HiMel   = HzToMel(_State->_HiFrequency);
        const double MelStep = (HiMel - LoMel) / (double) (_State->_MelBandCount + 1);

        double Mel = LoMel;

        for (size_t i = 0; i < Frequencies.size(); ++i, Mel += MelStep)
            Frequencies[i] = MelToHz(Mel);

        Frequencies.front() = _State->_LoFrequency;
        Frequencies.back()  = _State->_HiFrequency;
    }

    _FrequencyBands.resize(_State->_MelBandCount);

    size_t i = 0;

    for (frequency_band_t & fb: _FrequencyBands)
    {
        // Intentional overlap: Adjacent triangular Mel filters share their center and edge frequencies.
        fb.Lo  = Frequencies[i];
        fb.Mid = Frequencies[i + 1];
        fb.Hi  = Frequencies[i + 2];

        if (fb.Mid <= fb.Lo)
            fb.Mid = fb.Lo + 1.;

        if (fb.Hi <= fb.Mid)
            fb.Hi = fb.Mid + 1.;

        ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%d mel\n%.*f Hz", (int) HzToMel(fb.Mid), _GraphOptions->_XAxisDecimals, fb.Mid);

        fb.HasDarkBackground = true;

        ++i;
    }
}

/// <summary>
/// Generates logarithmically-spaced frequency bands.
/// </summary>
void analysis_t::GenerateLogFrequencyBands()
{
    constexpr std::size_t ScaleCount = 320;

    _FrequencyBands.resize(ScaleCount);

    const auto fMin = _State->_LoFrequency;
    const auto fMax = _State->_HiFrequency;

    size_t i = 0;

    for (frequency_band_t & fb: _FrequencyBands)
    {
        const auto t = (double) i / (double) (ScaleCount - 1);

        // Linear to logarithmic spacing.
        fb.Mid = fMin * std::pow(fMax / fMin, t); // Hz

        // Increase the bandwidth as the frequency rises.
        const auto Bandwidth = std::max(fb.Mid * 0.15, 1e-9); // Hz

        fb.Lo = std::max(fb.Mid - Bandwidth, MinFrequency); // Hz
        fb.Hi = std::min(fb.Mid + Bandwidth, MaxFrequency); // Hz

        ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _GraphOptions->_XAxisDecimals, fb.Mid);

        fb.HasDarkBackground = true;

        ++i;
    }
}

#pragma endregion

#pragma region Spectrum

void analysis_t::SpectrumProcessing(const audio_chunk & chunk) noexcept
{
    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

    if ((Frames == nullptr) || (FrameCount == 0))
        return;

    if (_WindowFunction == nullptr)
        _WindowFunction = window_function_t::Create(_State->_WindowFunction, _State->_WindowParameter, _State->_WindowSkew, _State->_Truncate);

    switch (_State->_TransformMethod)
    {
        case TransformMethod::FFT:
        {
            if (_FFTAnalyzer == nullptr)
            {       
                if (_BrownPucketteKernel == nullptr)
                    _BrownPucketteKernel = window_function_t::Create(_State->_KernelShape, _State->_KernelShapeParameter, _State->_KernelAsymmetry, _State->_Truncate);

                _FFTAnalyzer = new fft_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, _State->_BinCount, *_WindowFunction, *_BrownPucketteKernel);
            }

            _FFTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphOptions->_ActiveChannelMask, _FrequencyBands);
            break;
        }

        case TransformMethod::CQT:
        {
            if (_CQTAnalyzer == nullptr)
                _CQTAnalyzer = new cqt_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, *_WindowFunction);

            _CQTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphOptions->_ActiveChannelMask, _FrequencyBands);
            break;
        }

        case TransformMethod::SWIFT:
        {
            if (_SWIFTAnalyzer == nullptr)
            {
                _SWIFTAnalyzer = new swift_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig);

                _SWIFTAnalyzer->Initialize(_FrequencyBands);
            }

            _SWIFTAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphOptions->_ActiveChannelMask, _FrequencyBands);
            break;
        }

        case TransformMethod::AnalogStyle:
        {
            if (_AnalogStyleAnalyzer == nullptr)
            {
                _AnalogStyleAnalyzer = new analog_style_analyzer_t(_State, _SampleRate, _ChannelCount, _ChannelConfig, *_WindowFunction);

                _AnalogStyleAnalyzer->Initialize(_FrequencyBands);
            }

            _AnalogStyleAnalyzer->AnalyzeSamples(Frames, FrameCount, _GraphOptions->_ActiveChannelMask, _FrequencyBands);
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

    // From here on frequency_band_t::Value is guaranteed to be in the range [0, 1].
/*
{
    for (auto & fb : _FrequencyBands)
        fb.Value = 0.;

    _FrequencyBands.front().Value = .5;
    _FrequencyBands.back() .Value = .5;
}
*/
}

#pragma region Acoustic Weighting

/// <summary>
/// Applies acoustic weighting to the frequencies of the spectrum.
/// </summary>
void analysis_t::ApplyAcousticWeighting() noexcept
{
    assert((_SampleRate != 0) && (_State->_BinCount != 0));

    const double BinWidth = (double) _SampleRate / (double) _State->_BinCount;
    const double Offset   = _State->_FrequencyShift * BinWidth;

    for (frequency_band_t & fb : _FrequencyBands)
    {
        const double Frequency = fb.Mid + Offset;

        if (Frequency <= 0.)
            continue;

        const double Weight = GetWeight(Frequency);

        if (!std::isfinite(Weight) || Weight < 0.)
            continue;

        fb.RawValue *= Weight;
    }
}

/// <summary>
/// Gets the weight that needs to be applied to the specified frequency.
/// </summary>
double analysis_t::GetWeight(double f) const noexcept
{
    const double a = GetFrequencyTilt (f, _State->_FrequencyTilt, _State->_FrequencyTiltPivot);
    const double b = Equalize         (f, _State->_EqualizationAmount, _State->_EqualizationDepth, _State->_EqualizationFreqScale);
    const double c = GetAcousticWeight(f, _State->_WeightingType, _State->_WeightingAmount);

    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || a < 0. || b < 0. || c < 0.)
        return 1.; // Neutral

    return a * b * c;
}

/// <summary>
/// Gets the frequency tilt.
/// </summary>
inline double analysis_t::GetFrequencyTilt(double f, double amount, double offset) noexcept
{
    assert((f > 0.) && (offset > 0.));

    return std::pow(f / offset, amount / 6.020599913);
}

/// <summary>
/// Equalizes the weight.
/// </summary>
inline double analysis_t::Equalize(double f, double amount, double depth, double offset) noexcept
{
    const double pos = f * depth / offset;
    const double bias = std::pow(1.0025, -pos) * 0.04;

    return std::pow((10. * std::log10(1. + bias + (pos + 1.) * (9. - bias) / depth)), amount / 6.);
}

/// <summary>
/// Gets the weight for the specified frequency.
/// </summary>
inline double analysis_t::GetAcousticWeight(double f, WeightingType weightType, double weightAmount) noexcept
{
    constexpr double F1 =     20.6;
    constexpr double F2 =    107.7;
    constexpr double F3 =    737.9;
    constexpr double F4 = 12'194.0;

    constexpr double F5 =    158.5;

    const double f2 = f * f;
    const double f3 = f2 * f;
    const double f4 = f3 * f;
    const double f5 = f4 * f;
    const double f6 = f5 * f;

    switch (weightType)
    {
        default:

        case WeightingType::None:
            return 1.;

        case WeightingType::AWeighting:
        {
            constexpr double Normalization = 1.2588966; // std::pow(10., 2.0 / 20.);

            return std::pow(Normalization * (F4 * F4) * f4 / ((f2 + (F1 * F1)) * std::sqrt((f2 + (F2 * F2)) * (f2 + (F3 * F3))) * (f2 + (F4 * F4))), weightAmount);
        }

        case WeightingType::BWeighting:
        {
            constexpr double Normalization = 1.019764760044717; // std::pow(10., 0.17 / 20.);

            return std::pow(Normalization * (F4 * F4) * f3 / ((f2 + (F1 * F1)) * std::sqrt( f2 + (F5 * F5))                     * (f2 + (F4 * F4))), weightAmount);
        }

        case WeightingType::CWeighting:
        {
            constexpr double Normalization = 1.0069316688518042; // std::pow(10., 0.06 / 20.);

            return std::pow(Normalization * (F4 * F4) * f2 / ((f2 + (F1 * F1))                                                  * (f2 + (F4 * F4))), weightAmount);
        }

        case WeightingType::DWeighting:
            return std::pow(f / 6.8966888496476e-5 * std::sqrt(((1'037'918.48 - f2) * (1'037'918.48 - f2) + 1'080'768.16 * f2) / ((9'837'328. - f2) * (9'837'328. - f2) + (11'723'776. * f2)) / ((f2 + 79'919.29) * (f2 + 1'345'600.))), weightAmount);

        case WeightingType::MWeighting:
        {
            const double h1 = (-4.737338981378384e-24 * f6) + (2.043828333606125e-15 * f4) - (1.363894795463638e-7 * f2) + 1.;
            const double h2 = ( 1.306612257412824e-19 * f5) - (2.118150887518656e-11 * f3) + (5.559488023498642e-4 * f);

            return std::pow(8.128305161640991 * 1.246332637532143e-4 * f / std::hypot(h1, h2), weightAmount);
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
        fb.Value = std::clamp(_GraphOptions->ScaleAmplitude(fb.RawValue), 0., 1.);
}

/// <summary>
/// Normalizes the amplitude and applies average smoothing.
/// </summary>
void analysis_t::NormalizeWithAverageSmoothing(double factor) noexcept
{
    for (frequency_band_t & fb : _FrequencyBands)
        fb.Value = std::clamp((fb.Value * factor) + (::isfinite(fb.RawValue) ? _GraphOptions->ScaleAmplitude(fb.RawValue) * (1.0 - factor) : 0.), 0., 1.);
}

/// <summary>
/// Normalizes the amplitude and applies peak smoothing.
/// </summary>
void analysis_t::NormalizeWithPeakSmoothing(double factor) noexcept
{
    for (frequency_band_t & fb : _FrequencyBands)
        fb.Value = std::clamp(std::max(fb.Value * factor, std::isfinite(fb.RawValue) ? _GraphOptions->ScaleAmplitude(fb.RawValue) : 0.), 0., 1.);
}

#pragma endregion

#pragma endregion

#pragma region Peak/RMS Meter / Balance/Correlation Meter

/// <summary>
/// Process the chunk data for the peak and the level meter.
/// </summary>
void analysis_t::MeterProcessing(const audio_chunk & chunk) noexcept
{
    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount     = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = chunk.get_channel_count();

    if ((Frames == nullptr) || (FrameCount == 0))
        return;

    InitializePeakMeasurements(_ChannelMask);

    audio_sample BalanceSamples[2] = { };

    const audio_sample * CurrentFrame = Frames;

    for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
    {
        const audio_sample * Sample = CurrentFrame; // First sample of the current frame.

        size_t i = 0;
        size_t j = 0;

        uint32_t AvailableChannelMask = chunk.get_channel_config();                         // Mask containing the channels in the audio chunk.
        uint32_t ActiveChannelMask    = _GraphOptions->_ActiveChannelMask;                  // Mask containing the channels selected by the user for the level measuring.
        uint32_t PairedChannelMask    = ChannelPairs[(size_t) _GraphOptions->_ChannelPair]; // Mask containing the channels selected by the user for the balance measuring.

        while ((AvailableChannelMask & ActiveChannelMask) != 0)
        {
            if (AvailableChannelMask & 1)
            {
                if ((ActiveChannelMask & 1) && (i < _PeakMeasurements.size()))
                {
                    auto & m = _PeakMeasurements[i++];

                    const double Magnitude = std::abs((double) *Sample);

                    m.Peak = std::max(Magnitude, m.Peak); // Peak Magnitude
                    m.RMSTotal += Magnitude * Magnitude;   // Sum the instantaneous power.

                    if ((PairedChannelMask & 1) && (j < _countof(BalanceSamples)))
                        BalanceSamples[j++] = *Sample;
                }

                Sample++;
            }

            AvailableChannelMask >>= 1;
            ActiveChannelMask    >>= 1;
            PairedChannelMask    >>= 1;
        }

        _Left  += BalanceSamples[0] * BalanceSamples[0];
        _Right += BalanceSamples[1] * BalanceSamples[1];

        const double Mid  = (BalanceSamples[0] + BalanceSamples[1]) / 2.;
        const double Side = (BalanceSamples[0] - BalanceSamples[1]) / 2.;

        _Mid  += Mid  * Mid;
        _Side += Side * Side;

        CurrentFrame += ChannelCount;
    }

    _RMSFrameCount  += FrameCount;
    _RMSTimeElapsed += chunk.get_duration();

    // Normalize and smooth the peak values. https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
    for (auto & m : _PeakMeasurements)
    {
        m.Peak           = ToDecibel(m.Peak);
        m.NormalizedPeak = SmoothValue(NormalizeValue(m.Peak), m.NormalizedPeak);
    }

    // Has the RMS window elapsed yet?
    if (_RMSTimeElapsed < _State->_RMSWindow)
        return;

    for (auto & m : _PeakMeasurements)
    {
        // https://skippystudio.nl/2021/07/sound-intensity-and-decibels/
        m.RMS           = ToDecibel(std::sqrt(m.RMSTotal / (double) _RMSFrameCount)) + (_State->_HasRMSPlus3 ? dBCorrection : 0.);
        m.NormalizedRMS = SmoothValue(NormalizeValue(m.RMS), m.NormalizedRMS);

        // Reset the RMS window-dependent values.
        m.RMSTotal = 0.;
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

/// <summary>
/// Initializes the peak measurements before processing an audio chunk.
/// </summary>
void analysis_t::InitializePeakMeasurements(uint32_t activeChannelMask) noexcept
{
    if (_PeakActiveChannelMask != activeChannelMask)
    {
        // The chunk configuration has changed. Recreate the measurements.
        static constexpr const WCHAR * const ChannelNames[] =
        {
            L"FL", L"FR", L"FC",
            L"LFE",
            L"BL", L"BR", 
            L"FCL", L"FCR",
            L"BC", L"SL", L"SR", L"TC",
            L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
        };

        size_t i = 0;

        _PeakMeasurements.clear();

        for (uint32_t ActiveChannelMask = activeChannelMask; (ActiveChannelMask != 0) && (i < _countof(ChannelNames)); ActiveChannelMask >>= 1, ++i)
        {
            if (ActiveChannelMask & 1)
                _PeakMeasurements.push_back({ ChannelNames[i] });
        }

        _PeakActiveChannelMask = activeChannelMask;

        ResetRMSDependentValues();
    }
    else
    {
        // Reset only the peak level of each bar to -∞.
        for (auto & m : _PeakMeasurements)
            m.Peak = -std::numeric_limits<double>::infinity();
    }
}

#pragma endregion

#pragma region Oscilloscope

/// <summary>
/// Process the chunk data for the oscilloscope.
/// </summary>
void analysis_t::OscilloscopeProcessing(const audio_chunk & chunk) noexcept
{
    if (_State->_Downmix)
    {
        downmixer_t Downmixer;

        Downmixer(chunk, _GraphOptions->_ActiveChannelMask, _Chunk);
    }
    else
        _Chunk.copy(chunk, true);
}

#pragma endregion

#pragma region Bit Meter

/// <summary>
/// Process the chunk data for the bit meter.
/// </summary>
void analysis_t::BitMeterProcessing(const audio_chunk & chunk) noexcept
{
    const audio_sample * Frames = chunk.get_data();
    const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

    if ((Frames == nullptr) || (FrameCount == 0))
        return;

    InitializeBitMeasurements(_ChannelMask);

    const auto MaxInteger = (audio_sample) (1LL << (_State->_BitsPerInteger - 1));

    const audio_sample * CurrentFrame = Frames;

    for (size_t FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
    {
        const audio_sample * CurrentSample = CurrentFrame;

        size_t i = 0;

        uint32_t AvailableChannelMask = chunk.get_channel_config();         // Mask containing the channels in the audio chunk.
        uint32_t ActiveChannelMask    = _GraphOptions->_ActiveChannelMask;  // Mask containing the channels selected by the user for processing.

        while ((AvailableChannelMask & ActiveChannelMask) != 0)
        {
            if (AvailableChannelMask & 1)
            {
                if ((ActiveChannelMask & 1) && (i < _BitMeasurements.size()))
                {
                    uint64_t SampleBits;

                    if (_State->_BitMeterMode == BitMeterMode::FloatingPoint)
                    {
                        using sample_bits_t = std::conditional_t<sizeof(audio_sample) == 8, uint64_t, uint32_t>;

                        // Test pattern: 64-bit: 0b1101010101011001111111111111111111111111111111111111111111111001 / 32-bit: 0b11010101010000000000000000000001
                        SampleBits = std::bit_cast<sample_bits_t>(*CurrentSample);
                    }
                    else
                        SampleBits = (uint64_t) std::abs(*CurrentSample * MaxInteger);

                    for (auto & BitCount : _BitMeasurements[i].BitCounts)
                    {
                        if (SampleBits == 0)
                            break;

                        if (SampleBits & 1)
                            ++BitCount;

                        SampleBits >>= 1;
                    }

                    ++i;
                }

                ++CurrentSample;
            }

            AvailableChannelMask >>= 1;
            ActiveChannelMask    >>= 1;
        }

        CurrentFrame += _ChannelCount;
    }

    // Scale the bit counters to range [0, 1].
    for (auto & m : _BitMeasurements)
    {
        for (auto & BitCount : m.BitCounts)
            BitCount /= (double) FrameCount;

        // Reverse the array to get: Index 0 = Sign bit, Index 1 - 11 = Exponent bits, Index 12 - 63 = Mantissa bits
        std::reverse(m.BitCounts.begin(), m.BitCounts.end());
    }
}

/// <summary>
/// Initializes the bit measurements before processing an audio chunk.
/// </summary>
void analysis_t::InitializeBitMeasurements(uint32_t activeChannelMask) noexcept
{
    if (_BitActiveChannelMask != activeChannelMask)
    {
        // The chunk configuration has changed. Recreate the measurements, one per selected channel.
        static const WCHAR * ChannelNames[] =
        {
            L"FL", L"FR", L"FC",
            L"LFE",
            L"BL", L"BR", 
            L"FCL", L"FCR",
            L"BC", L"SL", L"SR", L"TC",
            L"TFL", L"TFC", L"TFR", L"TBL", L"TBC", L"TBR",
        };

        const size_t n = (size_t) ((_State->_BitMeterMode == BitMeterMode::FloatingPoint) ? audio_sample_size : _State->_BitsPerInteger);

        size_t i = 0;

        _BitMeasurements.clear();

        for (uint32_t SelectedChannels = activeChannelMask; (SelectedChannels != 0) && (i < _countof(ChannelNames)); SelectedChannels >>= 1, ++i)
        {
            if (SelectedChannels & 1)
                _BitMeasurements.push_back({ ChannelNames[i], n });
        }

        _BitActiveChannelMask = activeChannelMask;
    }
    else
    {
        for (auto & m : _BitMeasurements)
            std::fill(m.BitCounts.begin(), m.BitCounts.end(), 0.);
    }
}

#pragma endregion

#pragma region Goniometer

/// <summary>
/// Process the chunk data for the goniometer.
/// </summary>
void analysis_t::StereoMeterProcessing(const audio_chunk & chunk) noexcept
{
    _Chunk.copy(chunk, true);
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
