#pragma once

#include <algorithm>
#include <vector>

#include <d2d1.h>

#include <sdk/audio_chunk.h>

enum class ColorMode
{
    Static,
    RGB,
    MultiBand
};

enum class CrossoverMode
{
    OnePole,
    LinkwitzRiley4
};

/// <summary>
/// Biquad filter
/// </summary>
class biquad_t
{
public:
    enum class PassType
    {
        LowPass,
        HighPass
    };

    static constexpr double Butterworth2ndOrder = 0.70710678118;

    void Configure(PassType type, double hz, double sampleRate, double q = Butterworth2ndOrder) noexcept;
    double Process(double x);

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
    void Configure(double hz, double sampleRate) noexcept;
    void Process(double x, double & low, double & high);
    void Reset();

private:
    biquad_t _LP1, _LP2, _HP1, _HP2;
};

/// <summary>
/// Three band audio crossover filter.
/// </summary>
class audio_crossover_t
{
public:
    void Configure(double loFreq, double hiFreq, double sampleRate) noexcept;

    void SetCrossoverMode(CrossoverMode mode)
    {
        _CrossoverMode = mode;

        Reset();
    }

    void Process(double x, double & low, double & mid, double & high);
    void Reset();

private:
    CrossoverMode _CrossoverMode = CrossoverMode::LinkwitzRiley4;

    static constexpr double Woofer  =  220.;
    static constexpr double Tweeter = 2500.;

    double _LoFreq       = Woofer;
    double _HiFreq       = Tweeter;
    double _SampleRate = 48000.;

    double _LoAlpha = 0.;
    double _HiAlpha = 0.;

    double _LoLP = 0.;
    double _HiLP = 0.;

    lr4_crossover_t _LowMid;    // Low and everything lower
    lr4_crossover_t _MidHigh;   // High and everything higher
};

struct vertex_t
{
    float Center[2];
    float Corner[2];
    float Color[3];
    float Energy;
};

class audio_processor_t
{
public:
    void SetColorMode(ColorMode m)
    {
        _ColorMode = m;
    }

    ColorMode GetColorMode() const
    {
        return _ColorMode;
    }

    void SetCrossoverMode(CrossoverMode m);

    CrossoverMode GetCrossoverMode() const
    {
        return _CrossoverMode;
    }

    double GetCorrelation() const
    {
        return _Correlation;
    }

    std::vector<vertex_t> Process(const audio_chunk_impl & chunk, double energy, uint32_t selectedChannels, uint32_t pairedChannels) noexcept;

private:
    void Configure(uint32_t sampleRate);

    static void AddPoint(std::vector<vertex_t> & vertices, double left, double right, const D2D1_COLOR_F color, double energy) noexcept;

private:
    ColorMode _ColorMode = ColorMode::MultiBand;
    CrossoverMode _CrossoverMode = CrossoverMode::LinkwitzRiley4;

    uint32_t _SampleRate = 0;

    audio_crossover_t _CrossoverL;
    audio_crossover_t _CrossoverR;

    double _Correlation = 0.f;
    float _Rotation = 45.f;
};
