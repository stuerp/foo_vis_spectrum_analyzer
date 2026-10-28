
/** $VER: SpectrumProcessor.cpp (2026.10.28) P. Stuer - Implements an audio processor for the test visualization. **/

#include <pch.h>

#include "SpectrumProcessor.h"

#include "FrequencyScaler.h"

#include "FFTAnalyzer.h"
#include "CompositeFFTAnalyzer.h"
#include "CQTAnalyzer.h"
#include "SWIFTAnalyzer.h"
#include "AnalogStyleAnalyzer.h"

namespace Spectrum
{
    /// <summary>
    /// 
    /// </summary>
    void processor_t::Configure(const graph_options_t * options) noexcept
    {
        __super::Configure(options);

        switch (_State._FrequencyDistribution)
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

            case FrequencyDistribution::Decades:
                GenerateDecadeAlignedFrequencyBands();
                break;
        }
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Reset() noexcept
    {
        __super::Reset();

        if (_Analyzer)
            _Analyzer.reset();

        if (_BrownPucketteKernel)
            _BrownPucketteKernel.reset();

        if (WindowFunction)
            WindowFunction.reset();

        for (auto & fb : FrequencyBands)
            fb.Value = 0.;

        ResetPeaks();
    }

    /// <summary>
    /// 
    /// </summary>
    void processor_t::Process(const audio_chunk & chunk) noexcept
    {
        if (chunk.is_empty())
            return;

        if ((_SampleRate != chunk.get_sample_rate()) || (_ChannelCount != chunk.get_channel_count()) || (_ChannelConfig != chunk.get_channel_config()))
            Reset();

        _SampleRate       = chunk.get_sample_rate();
        _ChannelCount     = chunk.get_channel_count();
        _ChannelConfig    = chunk.get_channel_config();

        _ChannelMask      = _ChannelConfig & _Options->_ActiveChannelMask;

        if (_ChannelMask == 0)
            return; // None of the selected channels are present in this chunk.

        NyquistFrequency = _SampleRate / 2.;

        const audio_sample * Frames = chunk.get_data();
        const size_t FrameCount = chunk.get_sample_count(); // get_sample_count() actually returns the number of frames.

        CreateAnalyzer();

        _Analyzer->AnalyzeAudio(Frames, FrameCount, _Options->_ActiveChannelMask, FrequencyBands);

        // Filter the spectrum.
        if (_State._WeightingType != WeightingType::None)
            ApplyAcousticWeighting();

        // Smooth the spectrum.
        switch (_State._SmoothingMethod)
        {
            default:

            case SmoothingMethod::None:
            {
                Normalize();
                break;
            }

            case SmoothingMethod::Average:
            {
                NormalizeWithAverageSmoothing(_State._SmoothingFactor);
                break;
            }

            case SmoothingMethod::Peak:
            {
                NormalizeWithPeakSmoothing(_State._SmoothingFactor);
                break;
            }
        }

        // From here on frequency_band_t::Value is guaranteed to be in the range [0, 1].
    /*
    {
        for (auto & fb : FrequencyBands)
            fb.Value = 0.;

        FrequencyBands.front().Value = .5;
        FrequencyBands.back() .Value = .5;
    }
    */
    }

    /// <summary>
    /// Updates the peak values.
    /// </summary>
    void processor_t::UpdatePeaks(bool isStopped) noexcept
    {
        assert(_Options);

        const double Elapsed = _Chrono.Elapsed();

        const double AmplitudeRange = _Options->_AmplitudeHi - _Options->_AmplitudeLo;

        const double HoldTime = _State._HoldTime * (double) _State._TargetRefreshRate;
        const double FallRate = (_State._FallRate != 0) ? msc::Map(Elapsed, 0., AmplitudeRange / _State._FallRate, 0., 1.) : 0.;

        const bool IsAIMP = (_State._PeakMode == PeakMode::AIMP) || (_State._PeakMode == PeakMode::FadingAIMP);

        // Animate the spectrum peak value.
        for (auto & fb : FrequencyBands)
        {
            if (fb.Value >= fb.PeakValue)
            {
                if (IsAIMP)
                    fb.HoldTime += (fb.Value - fb.PeakValue) * HoldTime;
                else
                    fb.HoldTime = HoldTime;

                fb.PeakValue = fb.Value;
                fb.FallRate  = 0.;
                fb.Opacity   = 1.;
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
                    switch (_State._PeakMode)
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

        _Chrono.Reset();
    }

    /// <summary>
    /// Resets the FFT peaks.
    /// </summary>
    void processor_t::ResetPeaks() noexcept
    {
        for (auto & fb : FrequencyBands)
        {
            fb.PeakValue = 0.;
            fb.FallRate  = 0.;
            fb.Opacity   = 1.;
        }
    }

    /// <summary>
    /// Generates frequency bands using a linear distribution.
    /// </summary>
    void processor_t::GenerateLinearFrequencyBands()
    {
        assert(_State._BandCount != 0);

        const double MinScale = ScaleFrequency(_State._LoFrequency, _State._ScalingFunction, _State._SkewFactor);
        const double MaxScale = ScaleFrequency(_State._HiFrequency, _State._ScalingFunction, _State._SkewFactor);

        const double Bandwidth = ((((_State._TransformMethod == TransformMethod::FFT) || (_State._TransformMethod == TransformMethod::CompositeFFT)) &&
                                 (_State._MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State._TransformMethod == TransformMethod::CQT)) ? _State._Bandwidth : 0.5;

        FrequencyBands.resize(_State._BandCount);

        double i = 0.;

        const double MaxIndex = (double) (_State._BandCount - 1);

        for (frequency_band_t & fb: FrequencyBands)
        {
            const double LoIndex = std::clamp(i - Bandwidth, 0., MaxIndex);
            const double HiIndex = std::clamp(i + Bandwidth, 0., MaxIndex);

            fb.Low    = DescaleFrequency(msc::Map(LoIndex, 0., MaxIndex, MinScale, MaxScale), _State._ScalingFunction, _State._SkewFactor);
            fb.Center = DescaleFrequency(msc::Map(i,       0., MaxIndex, MinScale, MaxScale), _State._ScalingFunction, _State._SkewFactor);
            fb.High   = DescaleFrequency(msc::Map(HiIndex, 0., MaxIndex, MinScale, MaxScale), _State._ScalingFunction, _State._SkewFactor);

            assert(std::isfinite(fb.Low));
            assert(std::isfinite(fb.Center));
            assert(std::isfinite(fb.High));

            assert(fb.Low <= fb.Center && fb.Center <= fb.High);

            ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _Options->_XAxisDecimals, fb.Center);

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
    void processor_t::GenerateOctaveFrequencyBands()
    {
        assert(_State._TuningPitch > 0.); assert(_State._BandsPerOctave != 0);

        const double Root24 = std::exp2(1. / 24.); // 24 quarter tones (https://en.wikipedia.org/wiki/Quarter_tone)

        constexpr double C0 = 16.35; // Hz

        const double TuningOffset = (_State._TuningPitch > 0.) ? std::round(12.* std::log2(_State._TuningPitch / C0)) * 2. : 0.;  // Number of quarter-tone steps between C0 and the nearest equal-tempered semitone corresponding to the tuning frequency.
        const double C0Frequency  =  _State._TuningPitch * std::pow(Root24, -TuningOffset);                                        // Frequency of C0 tuned with the specified frequency (~16.35 Hz)

        const double NoteGroup = 24. / _State._BandsPerOctave;

        const double LoIndex = std::round(_State._LoNote * 2. / NoteGroup);
        const double HiIndex = std::round(_State._HiNote * 2. / NoteGroup);

        assert(LoIndex <= HiIndex);

        const double Bandwidth = ((((_State._TransformMethod == TransformMethod::FFT) || (_State._TransformMethod == TransformMethod::CompositeFFT)) && (_State._MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State._TransformMethod == TransformMethod::CQT)) ? _State._Bandwidth : 0.5;

        FrequencyBands.reserve((size_t) (HiIndex - LoIndex + 1.));

        static constexpr const WCHAR * NoteNames[] = { L"C", L"C#", L"D", L"D#", L"E", L"F", L"F#", L"G", L"G#", L"A", L"A#", L"B" };

        for (double i = LoIndex; i <= HiIndex; ++i)
        {
            frequency_band_t fb = 
            {
                C0Frequency * std::pow(Root24, (i - Bandwidth) * NoteGroup + _State._Transpose),
                C0Frequency * std::pow(Root24,  i              * NoteGroup + _State._Transpose),
                C0Frequency * std::pow(Root24, (i + Bandwidth) * NoteGroup + _State._Transpose),
            };

            assert(fb.Low <= fb.Center && fb.Center <= fb.High);

            const double f = NoteToFrequency(FrequencyToNote(fb.Center));

            // Pre-calculate the tooltip text and the band background color.
            {
                const uint32_t Note = (uint32_t) (i * (NoteGroup / 2.));

                const uint32_t n      = Note % (uint32_t) _countof(NoteNames);
                const uint32_t Octave = Note / (uint32_t) _countof(NoteNames);

                if (msc::InRange(f, fb.Low, fb.High))
                    ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%s%d\n%.*f Hz", NoteNames[n], Octave, _Options->_XAxisDecimals, fb.Center);
                else
                    ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _Options->_XAxisDecimals, fb.Center);

                fb.HasDarkBackground = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
            }

            FrequencyBands.push_back(fb);
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
    void processor_t::GenerateAveePlayerFrequencyBands()
    {
        assert(_State._BandCount > 1); assert(0. <= _State._Bandwidth && _State._Bandwidth <= 64.);

        const double Bandwidth = ((((_State._TransformMethod == TransformMethod::FFT) || (_State._TransformMethod == TransformMethod::CompositeFFT)) && (_State._MappingMethod == CoefficientMapping::TriangularFilterBank)) || (_State._TransformMethod == TransformMethod::CQT)) ? _State._Bandwidth : 0.5;

        FrequencyBands.resize(_State._BandCount);

        const double MaxIndex = (double) (_State._BandCount - 1);

        double i = 0.;

        for (frequency_band_t & fb : FrequencyBands)
        {
            const double LoIndex = std::clamp(i - Bandwidth, 0., MaxIndex);
            const double HiIndex = std::clamp(i + Bandwidth, 0., MaxIndex);

            fb.Low  = CalcBlendedLogLinearFrequency(_State._LoFrequency, _State._HiFrequency, LoIndex, MaxIndex, _State._SkewFactor);
            fb.Center = CalcBlendedLogLinearFrequency(_State._LoFrequency, _State._HiFrequency, i,       MaxIndex, _State._SkewFactor);
            fb.High  = CalcBlendedLogLinearFrequency(_State._LoFrequency, _State._HiFrequency, HiIndex, MaxIndex, _State._SkewFactor);

            assert(fb.Low <= fb.Center && fb.Center <= fb.High);

            ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f Hz", _Options->_XAxisDecimals, fb.Center);

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
    void processor_t::GenerateMelFrequencyBands()
    {
        // Generate the frequencies for each of the Mel points.
        std::vector<double> Frequencies(_State._MelBandCount + 2);
        {
            assert(_State._LoFrequency < _State._HiFrequency);

            const double LoMel   = HzToMel(_State._LoFrequency);
            const double HiMel   = HzToMel(_State._HiFrequency);
            const double MelStep = (HiMel - LoMel) / (double) (_State._MelBandCount + 1);

            double Mel = LoMel;

            for (size_t i = 0; i < Frequencies.size(); ++i, Mel += MelStep)
                Frequencies[i] = MelToHz(Mel);

            Frequencies.front() = _State._LoFrequency;
            Frequencies.back()  = _State._HiFrequency;
        }

        FrequencyBands.resize(_State._MelBandCount);

        size_t i = 0;

        for (frequency_band_t & fb: FrequencyBands)
        {
            // Intentional overlap: Adjacent triangular Mel filters share their center and edge frequencies.
            fb.Low  = Frequencies[i];
            fb.Center = Frequencies[i + 1];
            fb.High  = Frequencies[i + 2];

            if (fb.Center <= fb.Low)
                fb.Center = fb.Low + 1.;

            if (fb.High <= fb.Center)
                fb.High = fb.Center + 1.;

            ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%d mel\n%.*f Hz", (int) HzToMel(fb.Center), _Options->_XAxisDecimals, fb.Center);

            fb.HasDarkBackground = true;

            ++i;
        }
    }

    /// <summary>
    /// Generates frequency bands whose center frequencies are aligned to decade-dependent frequency intervals.
    /// </summary>
    void processor_t::GenerateDecadeAlignedFrequencyBands()
    {
        assert(_State._LoFrequency <= _State._HiFrequency);

        const size_t RequestedBandCount = (size_t) _State._BandCount;

        FrequencyBands.clear();

        if (RequestedBandCount == 0)
            return;

        std::vector<double> CenterFrequencies;

        CenterFrequencies.reserve(RequestedBandCount);

        const auto AddCenterFrequencies = [&](double minFrequency, double maxFrequency, double interval)
        {
            const double LoFrequency = std::max(_State._LoFrequency, minFrequency);
            const double HiFrequency = std::min(_State._HiFrequency, maxFrequency);

            if (LoFrequency > HiFrequency)
                return;

            double Frequency = std::ceil(LoFrequency / interval) * interval;

            while (Frequency <= HiFrequency)
            {
                CenterFrequencies.push_back(Frequency);

                Frequency += interval;
            }
        };

        AddCenterFrequencies(   1.0,   10.0 - std::numeric_limits<double>::epsilon(),   1.0);
        AddCenterFrequencies(  10.0,  100.0 - std::numeric_limits<double>::epsilon(),  10.0);
        AddCenterFrequencies( 100.0, 1000.0 - std::numeric_limits<double>::epsilon(), 100.0);
        AddCenterFrequencies(1000.0, _State._HiFrequency, 1000.0);

        // Every decade-aligned center is mandatory. Therefore, the requested band count may not be smaller than the number of mandatory anchors.
        if (CenterFrequencies.size() > RequestedBandCount)
            throw std::invalid_argument("Requested number of bands is too small");

        // If the range contains no decade-aligned frequency, create one initial center at the midpoint of the configured frequency range.
        if (CenterFrequencies.empty())
            CenterFrequencies.push_back(std::midpoint(_State._LoFrequency, _State._HiFrequency));

        // Add centers until the requested number of bands is reached. Each additional center splits the widest available interval. Existing decade-aligned centers are never removed or moved.
        while (CenterFrequencies.size() < RequestedBandCount)
        {
            size_t InsertIndex = 0;

            double WidestInterval = -1.0;

            for (size_t i = 0; i <= CenterFrequencies.size(); ++i)
            {
                const double Low  = (i == 0) ? _State._LoFrequency : CenterFrequencies[i - 1];
                const double High = (i == CenterFrequencies.size()) ? _State._HiFrequency : CenterFrequencies[i];

                const double Interval = High - Low;

                if (Interval > WidestInterval)
                {
                    WidestInterval = Interval;
                    InsertIndex = i;
                }
            }

            const double Low  = (InsertIndex == 0)                        ? _State._LoFrequency : CenterFrequencies[InsertIndex - 1];
            const double High = (InsertIndex == CenterFrequencies.size()) ? _State._HiFrequency : CenterFrequencies[InsertIndex];

            const double NewCenter = std::midpoint(Low, High);

            // Ensure the new center is distinct from both interval endpoints. Failure is only expected when no further distinct double can be represented inside the selected interval.
            if (!(Low < NewCenter && NewCenter < High))
                throw std::overflow_error("Unable to generate the requested number of distinct frequency-band centers");

            CenterFrequencies.insert(CenterFrequencies.begin() + static_cast<std::ptrdiff_t>(InsertIndex), NewCenter);
        }

        assert(CenterFrequencies.size() == RequestedBandCount);
        assert(std::is_sorted(CenterFrequencies.begin(), CenterFrequencies.end()));

        FrequencyBands.resize(CenterFrequencies.size());

        for (size_t i = 0; i < CenterFrequencies.size(); ++i)
        {
            frequency_band_t & fb = FrequencyBands[i];

            fb.Center = CenterFrequencies[i];

            if (i == 0)
                fb.Low = _State._LoFrequency;
            else
                fb.Low = std::midpoint(CenterFrequencies[i - 1], fb.Center);

            if (i + 1 == CenterFrequencies.size())
                fb.High = _State._HiFrequency;
            else
                fb.High = std::midpoint(fb.Center, CenterFrequencies[i + 1]);

            fb.Low  = std::clamp(fb.Low,  _State._LoFrequency, _State._HiFrequency);
            fb.High = std::clamp(fb.High, _State._LoFrequency, _State._HiFrequency);

            assert(std::isfinite(fb.Low));
            assert(std::isfinite(fb.Center));
            assert(std::isfinite(fb.High));

            assert(fb.Low <= fb.Center);
            assert(fb.Center <= fb.High);

            if (fb.Center >= 1'000.0)
            {
                const double CenterInKilohertz = fb.Center / 1'000.0;

                double IntegerPart = 0.;

                const double FractionalPart = std::modf(CenterInKilohertz, &IntegerPart);

                if (FractionalPart == 0.)
                    ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.0fk", CenterInKilohertz);
                else
                    ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*fk", _Options->_XAxisDecimals, CenterInKilohertz);
            }
            else
                ::StringCchPrintfW(fb.Label, _countof(fb.Label), L"%.*f", _Options->_XAxisDecimals, fb.Center);

            fb.HasDarkBackground = true;
        }
    }

    /// <summary>
    /// Creates and initializes the analyze.
    /// </summary>
    void processor_t::CreateAnalyzer() noexcept
    {
        if (!WindowFunction)
            WindowFunction = window_function_t::Create(_State.WindowFunction, _State._WindowParameter, _State._WindowSkew, _State._Truncate);

        if (!_Analyzer)
        {
            #pragma warning(disable: 4062) // enumerator 'TransformMethod::Invalid' in switch of enum 'TransformMethod' is not handled
            switch (_State._TransformMethod)
            {
                case TransformMethod::FFT:
                {
                    if (!_BrownPucketteKernel)
                        _BrownPucketteKernel = window_function_t::Create(_State._KernelShape, _State._KernelShapeParameter, _State._KernelAsymmetry, _State._Truncate);

                    _Analyzer = std::make_unique<fft_analyzer_t>(_State, _State._BinCount, _SampleRate, _ChannelCount, _ChannelConfig, *WindowFunction, *_BrownPucketteKernel);
                    break;
                }

                case TransformMethod::CQT:
                {
                    _Analyzer = std::make_unique<cqt_analyzer_t>(_State, _SampleRate, _ChannelCount, _ChannelConfig, *WindowFunction);
                    break;
                }

                case TransformMethod::SWIFT:
                {
                    auto Analyzer = std::make_unique<swift_analyzer_t>(_State, _SampleRate, _ChannelCount, _ChannelConfig);

                    Analyzer->Initialize(FrequencyBands);

                    _Analyzer = std::move(Analyzer);
                    break;
                }

                case TransformMethod::AnalogStyle:
                {
                    auto Analyzer = std::make_unique<analog_style_analyzer_t>(_State, _SampleRate, _ChannelCount, _ChannelConfig, *WindowFunction);

                    Analyzer->Initialize(FrequencyBands);

                    _Analyzer = std::move(Analyzer);
                    break;
                }

                case TransformMethod::CompositeFFT:
                {
                    if (!_BrownPucketteKernel)
                        _BrownPucketteKernel = window_function_t::Create(_State._KernelShape, _State._KernelShapeParameter, _State._KernelAsymmetry, _State._Truncate);

                    _Analyzer = std::make_unique<composite_fft_analyzer_t>(_State, _SampleRate, _ChannelCount, _ChannelConfig, *WindowFunction, *_BrownPucketteKernel);
                    break;
                }

            }
        }
    }

    /// <summary>
    /// Applies acoustic weighting to the frequencies of the spectrum.
    /// </summary>
    void processor_t::ApplyAcousticWeighting() noexcept
    {
        assert((_SampleRate != 0) && (_State._BinCount != 0));

        const double BinWidth = (double) _SampleRate / (double) _State._BinCount;
        const double Offset   = _State._FrequencyShift * BinWidth;

        for (frequency_band_t & fb : FrequencyBands)
        {
            const double Frequency = fb.Center + Offset;

            if (Frequency <= 0.)
                continue;

            const double Weight = GetWeight(Frequency);

            if (!std::isfinite(Weight) || Weight < 0.)
                continue;

            fb.Magnitude *= Weight;
        }
    }

    /// <summary>
    /// Gets the weight that needs to be applied to the specified frequency.
    /// </summary>
    double processor_t::GetWeight(double f) const noexcept
    {
        const double a = GetFrequencyTilt (f, _State._FrequencyTilt, _State._FrequencyTiltPivot);
        const double b = Equalize         (f, _State._EqualizationAmount, _State._EqualizationDepth, _State._EqualizationFreqScale);
        const double c = GetAcousticWeight(f, _State._WeightingType, _State._WeightingAmount);

        if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || a < 0. || b < 0. || c < 0.)
            return 1.; // Neutral

        return a * b * c;
    }

    /// <summary>
    /// Gets the frequency tilt.
    /// </summary>
    inline double processor_t::GetFrequencyTilt(double f, double amount, double offset) noexcept
    {
        assert((f > 0.) && (offset > 0.));

        return std::pow(f / offset, amount / 6.020599913);
    }

    /// <summary>
    /// Equalizes the weight.
    /// </summary>
    inline double processor_t::Equalize(double f, double amount, double depth, double offset) noexcept
    {
        const double pos = f * depth / offset;
        const double bias = std::pow(1.0025, -pos) * 0.04;

        return std::pow((10. * std::log10(1. + bias + (pos + 1.) * (9. - bias) / depth)), amount / 6.);
    }

    /// <summary>
    /// Gets the weight for the specified frequency.
    /// </summary>
    inline double processor_t::GetAcousticWeight(double f, WeightingType weightType, double weightAmount) noexcept
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

    /// <summary>
    /// Normalizes the raw amplitude for display without smoothing. Fully responsive but potentially flickery.
    /// </summary>
    void processor_t::Normalize() noexcept
    {
        for (frequency_band_t & fb : FrequencyBands)
        {
            assert(::isfinite(fb.Magnitude));

            const auto ScaledValue = _Options->ScaleAmplitude(fb.Magnitude);

            fb.Value = std::clamp(ScaledValue, 0., 1.);
        }
    }

    /// <summary>
    /// Normalizes the amplitude for display with average smoothing. Gradual attack and decay of the amplitude.
    /// </summary>
    void processor_t::NormalizeWithAverageSmoothing(double strength) noexcept
    {
        for (frequency_band_t & fb : FrequencyBands)
        {
            const auto ScaledValue = ::isfinite(fb.Magnitude) ? _Options->ScaleAmplitude(fb.Magnitude) * (1.0 - strength) : 0.;

            fb.Value = std::clamp((fb.Value * strength) + ScaledValue, 0., 1.);
        }
    }

    /// <summary>
    /// Normalizes the amplitude for display with peak smoothing. Instant attack with exponential peak and gradual decay of the amplitude. Preserves peaks and transients.
    /// </summary>
    void processor_t::NormalizeWithPeakSmoothing(double strength) noexcept
{
    for (frequency_band_t & fb : FrequencyBands)
    {
        const auto ScaledValue = std::isfinite(fb.Magnitude) ? _Options->ScaleAmplitude(fb.Magnitude) : 0.;

        fb.Value = std::clamp(std::max(fb.Value * strength, ScaledValue), 0., 1.);
    }
}
}
