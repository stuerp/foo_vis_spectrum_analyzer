
/** $VER: CrossoverFilter.cpp (2026.09.20) P. Stuer - Implements an audio crossover filter. **/

#pragma once

/// <summary>
/// Biquad filter
/// </summary>
class biquad_t
{
public:
    enum class PassType
    {
        Unknown = 0,

        LowPass,
        HighPass,

        Min = LowPass,
        Max = HighPass
    };

    static constexpr double Butterworth2ndOrder = 0.70710678118;

    HRESULT Configure(PassType type, double frequency, double sampleRate, double q = Butterworth2ndOrder) noexcept;
    double Process(double amplitude) noexcept;

    void Reset() noexcept
    {
        _Z1 = _Z2 = 0.;
    }

private:
    double _B0 = 1.;
    double _B1 = 0.;
    double _B2 = 0.;

    double _A1 = 0.;
    double _A2 = 0.;

    double _Z1 = 0.;
    double _Z2 = 0.;
};

/// <summary>
/// 4-th order Linkwitz-Riley crossover, https://en.wikipedia.org/wiki/Linkwitz%E2%80%93Riley_filter
/// </summary>
class lr4_crossover_t
{
public:
    HRESULT Configure(double frequency, double sampleRate) noexcept;
    void Process(double amplitude, double & lowBand, double & highBand) noexcept;
    void Reset() noexcept;

private:
    double _Frequency  = 0.;
    double _SampleRate = 0.;

    biquad_t _LP1, _LP2, _HP1, _HP2;
};

/// <summary>
/// Three band audio crossover filter.
/// </summary>
class audio_crossover_t
{
public:
    HRESULT Configure(double loFreq, double hiFreq, double sampleRate) noexcept;

    enum class CrossoverMode
    {
        None = 0,

        FirstOrder,
        LinkwitzRiley4,

        Min = None,
        Max = LinkwitzRiley4,
    };

    HRESULT SetCrossoverMode(CrossoverMode mode) noexcept
    {
        if (mode == _Mode)
            return S_FALSE;

        _Mode = mode;

        Reset();

        return S_OK;
    }

    void Process(double amplitude, double & lowBand, double & midBand, double & highBand) noexcept;
    void Reset() noexcept;

private:
    CrossoverMode _Mode = CrossoverMode::LinkwitzRiley4;

    double _LoFreq     = 0.;
    double _HiFreq     = 0.;
    double _SampleRate = 0.;

    double _LoAlpha = 0.;
    double _HiAlpha = 0.;

    double _LoLP    = 0.;
    double _HiLP    = 0.;

    lr4_crossover_t _LowMid;    // Low and everything lower
    lr4_crossover_t _MidHigh;   // High and everything higher
};
