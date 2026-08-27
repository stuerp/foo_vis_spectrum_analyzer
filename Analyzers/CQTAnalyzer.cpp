
/** $VER: CQTAnalyzer.cpp (2026.08.19) P. Stuer - Based on TF3RDL's Constant-Q analyzer, https://codepen.io/TF3RDL/pen/poQJwRW **/

#include "pch.h"
#include "CQTAnalyzer.h"

#include <map>

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
cqt_analyzer_t::cqt_analyzer_t(const state_t * state, uint32_t sampleRate, uint32_t channelCount, uint32_t channelSetup, const window_function_t & windowFunction) : analyzer_t(state, sampleRate, channelCount, channelSetup, windowFunction)
{
}

/// <summary>
/// Calculates the Constant-Q Transform on the sample data and returns the frequency bands using a variable-window Goertzel bank with adaptive decimation.
/// Each frequency band chooses its own observation length and sampling period.
/// </summary>
#ifdef v1
bool cqt_analyzer_t::AnalyzeSamples(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
    if ((frames == nullptr) || (frameCount == 0) || (_ChannelCount == 0) || (_SampleRate == 0))
        return false;

    const size_t SampleCount = frameCount * _ChannelCount;
    const double FrequencyResolution = (double) _SampleRate / (double) SampleCount; // Fs / N

    const bool UseGranularSamplingPeriod = false;
    const bool UseGranularBandwidth = true;

    for (frequency_band_t & fb : frequencyBands)
    {
        const double Bandwidth  = std::abs(fb.Hi - fb.Lo) + (FrequencyResolution * _State->_CQTBandwidthOffset);
        const double TimeLength = std::min(1. / Bandwidth, 1. / FrequencyResolution);

        double SamplingPeriod = std::max(1., std::trunc(((double) _SampleRate * _State->_CQTDownSample) / (fb.Mid + TimeLength)));

        if (!UseGranularSamplingPeriod)
            SamplingPeriod = std::pow(2., std::trunc(std::log2(SamplingPeriod)));

        const double KTerm = fb.Mid * SamplingPeriod;                // Frequency of interest
        const double Omega = 2. * M_PI * KTerm / (double) _SampleRate;  // ω
        const double Coeff = 2. * std::cos(Omega);

        double BandSampleCount = TimeLength * (double) _SampleRate * _ChannelCount;

        if (!UseGranularBandwidth)
            BandSampleCount = std::min(std::trunc(std::pow(2., std::round(std::log2(BandSampleCount)))), (double) SampleCount);

        const double Offset = std::trunc(((double) SampleCount - BandSampleCount) * (0.5 + _State->_CQTAlignment / 2.));

        const double LoIdx = Offset;
        const double HiIdx = LoIdx + std::trunc(BandSampleCount) - 1.;

        double f1 = 0.;
        double f2 = 0.;

        double Norm = 0.;

        for (double Idx = std::trunc(LoIdx / SamplingPeriod); Idx <= std::trunc(HiIdx / SamplingPeriod); Idx += _ChannelCount)
        {
            const double x = (((Idx * SamplingPeriod) - LoIdx) / (HiIdx - LoIdx) * 2.) - 1.;
            const double w = _WindowFunction(x);

            const size_t i = (size_t) (Idx * SamplingPeriod);

            const double s = ((i < SampleCount - _ChannelCount) ? (AverageSamples(&frames[i], selectedChannels) * w) : 0.) + (Coeff * f1) - f2;

            Norm += w;

            f2 = f1;
            f1 = s;
        }

        const double Magnitude = std::sqrt((f1 * f1) + (f2 * f2) - (Coeff * f1 * f2));

        fb.RawValue = Magnitude / Norm; // Power
    }

    return true;
}
#endif
#ifdef v2
bool cqt_analyzer_t::AnalyzeSamples(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
    if ((frames == nullptr) || (frameCount == 0) || (_ChannelCount == 0) || (_SampleRate == 0))
        return false;

    const size_t SampleCount = frameCount * _ChannelCount;
    const double FrequencyResolution = (double) _SampleRate / (double) SampleCount; // Fs / N

    constexpr bool UseGranularSamplingPeriod = false;
    constexpr bool UseGranularBandwidth = true;

    // Precompute the selected-channel average once.
    std::vector<audio_sample> MonoSamples(SampleCount, 0.);

    for (size_t i = 0; i + _ChannelCount <= SampleCount; i += _ChannelCount)
        MonoSamples[i] = (double) AverageSamples(&frames[i], selectedChannels);

    for (frequency_band_t & fb : frequencyBands)
    {
        const double Bandwidth = std::abs(fb.Hi - fb.Lo) + (FrequencyResolution * _State->_CQTBandwidthOffset);

        if (Bandwidth <= 0.)
        {
            fb.RawValue = 0.;
            continue;
        }

        const double MaxBlockDuration = (double) SampleCount / (double)(_SampleRate * _ChannelCount);

        const double TimeLength = std::min(1. / Bandwidth, MaxBlockDuration); // in seconds

        if (TimeLength <= 0.)
        {
            fb.RawValue = 0.;
            continue;
        }

//      double SamplingPeriodFloat = std::max(1., std::trunc(((double) _SampleRate * _State->_CQTDownSample) / (fb.Mid + TimeLength)));
        double SamplingPeriodFloat = std::max(1., std::trunc(((double) _SampleRate * _State->_CQTDownSample) / fb.Hi));

        if (!UseGranularSamplingPeriod)
            SamplingPeriodFloat = std::pow(2., std::trunc(std::log2(SamplingPeriodFloat)));

        size_t SamplingPeriod = (size_t) std::max(1., SamplingPeriodFloat);

        // Goertzel coefficient. Because the signal is effectively sampled every samplingPeriod frames, the effective sample rate is Fs / samplingPeriod.
        const double Omega = 2. * M_PI * fb.Mid * (double) SamplingPeriod / (double) _SampleRate;

        const double Cos   = std::cos(Omega);
        const double Sin   = std::sin(Omega);
        const double Coeff = 2. * Cos;

        double BandSampleCountFloat = TimeLength * (double) _SampleRate * (double) _ChannelCount;

        if (!UseGranularBandwidth)
            BandSampleCountFloat = std::min(std::trunc(std::pow(2., std::round(std::log2(BandSampleCountFloat)))), (double) SampleCount);

        size_t BandSampleCount = (size_t) std::clamp(std::trunc(BandSampleCountFloat), (double) _ChannelCount, (double) SampleCount);

        // Keep the analysis region frame-aligned.
        BandSampleCount -= BandSampleCount % _ChannelCount;

        if (BandSampleCount < _ChannelCount)
        {
            fb.RawValue = 0.;
            continue;
        }

        const double Alignment = std::clamp(_State->_CQTAlignment, -1., 1.);

        const double OffsetFloat = std::trunc((double)(SampleCount - BandSampleCount) * (0.5 + Alignment * 0.5));
        size_t Offset = (size_t) std::clamp(OffsetFloat, 0., (double)(SampleCount - BandSampleCount));

        // Keep the offset frame-aligned.
        Offset -= Offset % _ChannelCount;

        const size_t LoIdx = Offset;
        const size_t HiIdx = Offset + BandSampleCount - _ChannelCount;

        if (HiIdx <= LoIdx || HiIdx >= SampleCount)
        {
            fb.RawValue = 0.0;
            continue;
        }

        // Goertzel state
        double f1 = 0.0;
        double f2 = 0.0;

        double WindowSum = 0.0;

        const size_t Step = SamplingPeriod * _ChannelCount;

        for (size_t i = LoIdx; i <= HiIdx; i += Step)
        {
            const double Position = (double)(i - LoIdx) / (double)(HiIdx - LoIdx);

            const double x = Position * 2. - 1.;
            const double w = _WindowFunction(x);

            const double Sample = MonoSamples[i] * w;

            const double s = Sample + (Coeff * f1) - f2;

            f2 = f1;
            f1 = s;

            WindowSum += w;

            if (HiIdx - i < Step)
                break;
        }

        if (WindowSum <= 0.0)
        {
            fb.RawValue = 0.0;
            continue;
        }

        const double Real = f1 - f2 * Cos;
        const double Imag =      f2 * Sin;

        const double Magnitude = std::hypot(Real, Imag);

        fb.RawValue = Magnitude / WindowSum;
    }

    return true;
}
#endif
bool cqt_analyzer_t::AnalyzeSamples(const audio_sample * frames, size_t frameCount, uint32_t selectedChannels, frequency_bands_t & frequencyBands) noexcept
{
    if ((frames == nullptr) || (frameCount == 0) || (_ChannelCount == 0) || (_SampleRate == 0))
        return false;

    const double FrequencyResolution = (double) _SampleRate / (double) frameCount; // Fs / frameCount

    constexpr bool UseGranularSamplingPeriod = false;
    constexpr bool UseGranularBandwidth = true;

    // Precompute the selected-channel average once, one value per audio frame.
    if (_MonoSamples.size() != frameCount)
        _MonoSamples.resize(frameCount);
/*
    for (size_t Frame = 0; Frame < frameCount; ++Frame)
        _MonoSamples[Frame] = Downmix(&frames[Frame * _ChannelCount], selectedChannels);
*/
    for (size_t SrcFrame = 0, DstFrame = 0; DstFrame < frameCount; SrcFrame += _ChannelCount, ++DstFrame)
        _MonoSamples[DstFrame] = Downmix(&frames[SrcFrame], selectedChannels);

    for (frequency_band_t & fb : frequencyBands)
    {
        // Calculate the band width (in Hz).
        const double Bandwidth = std::abs(fb.Hi - fb.Lo) + (FrequencyResolution * _State->_CQTBandwidthOffset);

        if (Bandwidth <= 0.)
        {
            fb.RawValue = 0.;
            continue;
        }

        // Calculate frame duration (in seconds).
        const double FrameDuration = std::min(1. / Bandwidth, (double) frameCount / (double) _SampleRate);

        if (FrameDuration <= 0.)
        {
            fb.RawValue = 0.;
            continue;
        }

        // Calculate the number of frames per band.
        double BandFrameCountFloat = FrameDuration * (double) _SampleRate;

        if (!UseGranularBandwidth)
            BandFrameCountFloat = std::min(std::trunc(std::pow(2., std::round(std::log2(BandFrameCountFloat)))), (double) frameCount);

        const size_t BandFrameCount = (size_t) std::clamp(std::trunc(BandFrameCountFloat), 1., (double) frameCount);

        // Calculate the optimal sampling period.
        double SamplingPeriodFloat = std::max(1., std::trunc(((double) _SampleRate * _State->_CQTDownSample) / (fb.Mid + Bandwidth)));

        if (!UseGranularSamplingPeriod)
            SamplingPeriodFloat = std::pow(2., std::trunc(std::log2(SamplingPeriodFloat)));

        size_t SamplingPeriod = (size_t) std::max(1., SamplingPeriodFloat);

        // Goertzel coefficient. Effective sample rate is Fs / SamplingPeriod.
        const double Omega = 2. * M_PI * fb.Mid * (double) SamplingPeriod / (double) _SampleRate;

        const double Cos   = std::cos(Omega);
        const double Sin   = std::sin(Omega);
        const double Coeff = 2. * Cos;

        const double Alignment = std::clamp(_State->_CQTAlignment, -1., 1.);

        const double OffsetFloat = std::trunc((double)(frameCount - BandFrameCount) * (0.5 + Alignment * 0.5));
        const size_t Offset = (size_t) std::clamp(OffsetFloat, 0., (double)(frameCount - BandFrameCount));

        const size_t LoSample = Offset;
        const size_t HiSample = Offset + BandFrameCount - 1;

        if (HiSample <= LoSample || HiSample >= frameCount)
        {
            fb.RawValue = 0.;
            continue;
        }

        double f1 = 0.;
        double f2 = 0.;

        double WindowSum = 0.;

        for (size_t SampleIndex = LoSample; SampleIndex <= HiSample; SampleIndex += SamplingPeriod)
        {
            const double Position = (double)(SampleIndex - LoSample) / (double)(HiSample - LoSample);

            const double x = Position * 2. - 1.;
            const double w = _WindowFunction(x);

            const auto Sample = (audio_sample) _MonoSamples[SampleIndex] * w;

            const double s = Sample + (Coeff * f1) - f2;

            f2 = f1;
            f1 = s;

            WindowSum += w;

            if (HiSample - SampleIndex < SamplingPeriod)
                break;
        }

        if (WindowSum <= 0.)
        {
            fb.RawValue = 0.;
            continue;
        }

        const double Real = f1 - f2 * Cos;
        const double Imag =      f2 * Sin;

        const double Magnitude = std::hypot(Real, Imag);

        fb.RawValue = Magnitude / WindowSum;
    }

    return true;
}
