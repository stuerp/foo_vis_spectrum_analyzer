
/** $VER: AudioProcessor.h (2026.09.20) P. Stuer - Implements an audio processor for the goniometer. **/

#pragma once

#include "CrossoverFilter.h"

#include <vector>

#include <d2d1.h>
#include <sdk/audio_chunk.h>

struct point_t
{
    float x;
    float y;
    D2D1_COLOR_F Color;
};

class audio_processor_t
{
public:
    audio_processor_t() = default;

    audio_processor_t(const audio_processor_t &) = delete;
    audio_processor_t & operator=(const audio_processor_t &) = delete;
    audio_processor_t(audio_processor_t &&) = delete;
    audio_processor_t & operator=(audio_processor_t &&) = delete;

    virtual ~audio_processor_t() = default;

    enum class ColorMode
    {
        Mono = 0,
        RGB,
        Triband,

        Min = Mono,
        Max = Triband,
    };

    HRESULT SetColorMode(ColorMode mode) noexcept
    {
        _ColorMode = mode;

        return S_OK;
    }

    ColorMode GetColorMode() const noexcept
    {
        return _ColorMode;
    }

    HRESULT SetMonoColor(const D2D1_COLOR_F & color) noexcept
    {
        _MonoColor = color;

        return S_OK;
    }

    HRESULT SetTriBandColors(const D2D1_COLOR_F & lowColor, const D2D1_COLOR_F & midColor, const D2D1_COLOR_F & highColor) noexcept
    {
        _LowColor  = lowColor;
        _MidColor  = midColor;
        _HighColor = highColor;

        return S_OK;
    }

    HRESULT SetCrossoverMode(crossover_filter_t::Mode mode) noexcept;

    crossover_filter_t::Mode GetCrossoverMode() const noexcept
    {
        return _CrossoverMode;
    }

    void Process(const audio_chunk_impl & chunk, uint32_t selectedChannels, uint32_t pairedChannels, double loFreq, double hiFreq) noexcept;

private:
    HRESULT Configure(double loFreq, double hiFreq, double sampleRate) noexcept;

    void AddPoint(double left, double right, const D2D1_COLOR_F color, double opacity) noexcept;

public:
    std::vector<point_t> _Points;
    size_t _PointCount = 0;

private:
    ColorMode _ColorMode = ColorMode::Triband;
    crossover_filter_t::Mode _CrossoverMode = crossover_filter_t::Mode::LinkwitzRiley4;

    double _LoFreq = 0.;
    double _HiFreq = 0.;
    double _SampleRate = 0.;

    static constexpr double _EnergyThreshold = 1e-9; // -90 dB
//  static constexpr double _EnergyThreshold = 1e-8; // -80 dB
//  static constexpr double _EnergyThreshold = 1e-6; // -60 dB

    const double _LowGain  = 1.30;
    const double _MidGain  = 1.10;
    const double _HighGain = 1.45;

    D2D1_COLOR_F _MonoColor = D2D1::ColorF( .10f, 1.00f,  .45f); // Green

    D2D1_COLOR_F _LowColor  = D2D1::ColorF(1.00f,  .12f,  .04f); // Red
    D2D1_COLOR_F _MidColor  = D2D1::ColorF( .08f, 1.00f,  .25f); // Green
    D2D1_COLOR_F _HighColor = D2D1::ColorF( .08f,  .35f, 1.00f); // Blue

    crossover_filter_t _CrossoverL;
    crossover_filter_t _CrossoverR;

    double _Correlation = 0.f;
};
