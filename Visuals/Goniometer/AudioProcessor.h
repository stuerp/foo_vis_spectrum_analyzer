
/** $VER: AudioProcessor.h (2026.09.22) P. Stuer - Implements an audio processor for the goniometer. **/

#pragma once

#include "CrossoverFilter.h"
#include "State.h"

#include <vector>

#include <d2d1.h>
#include <sdk/audio_chunk.h>

struct point_t
{
    FLOAT x;
    FLOAT y;
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

    HRESULT SetColorMode(goniometer::ColorMode mode) noexcept
    {
        _ColorMode = mode;

        return S_OK;
    }

    goniometer::ColorMode GetColorMode() const noexcept
    {
        return _ColorMode;
    }

    HRESULT SetMonoColor(const D2D1_COLOR_F & color) noexcept
    {
        _MonoColor = color;

        _MonoColor.a = (FLOAT) _MidOpacity;

        return S_OK;
    }

    HRESULT SetTriBandColors(const D2D1_COLOR_F & lowColor, const D2D1_COLOR_F & midColor, const D2D1_COLOR_F & highColor) noexcept
    {
        _LowColor  = lowColor;
        _MidColor  = midColor;
        _HighColor = highColor;

        _LowColor.a  = (FLOAT) _LowOpacity;
        _MidColor.a  = (FLOAT) _MidOpacity;
        _HighColor.a = (FLOAT) _HighOpacity;

        return S_OK;
    }

    HRESULT SetCrossoverMode(crossover::Mode mode) noexcept;

    HRESULT SetVisualGain(double lowVisualGain, double midVisualGain, double highVisualGain) noexcept
    {
        constexpr double PointOpacity = 0.018;

        _LowOpacity  = PointOpacity * std::pow(10.0, lowVisualGain  / 20.0);
        _MidOpacity  = PointOpacity * std::pow(10.0, midVisualGain  / 20.0);
        _HighOpacity = PointOpacity * std::pow(10.0, highVisualGain / 20.0);

        _MonoColor.a = (FLOAT) _MidOpacity;

        _LowColor.a  = (FLOAT) _LowOpacity;
        _MidColor.a  = (FLOAT) _MidOpacity;
        _HighColor.a = (FLOAT) _HighOpacity;

        return S_OK;
    }

    void Initialize(state_t * state) noexcept
    {
        _State = state;
    }

    void Process(const audio_chunk_impl & chunk, uint32_t selectedChannels, uint32_t pairedChannels, double loFreq, double hiFreq) noexcept;

    void Reset() noexcept
    {
        _PointCount = 0;
    }

private:
    HRESULT Configure(double loBand, double hiBand, double sampleRate) noexcept;

    inline void AddPoint(double left, double right, const D2D1_COLOR_F color) noexcept;

public:
    std::vector<point_t> _Points;
    size_t _PointCount = 0;

private:
    state_t * _State = nullptr;

    goniometer::ColorMode _ColorMode = goniometer::ColorMode::Triband;
    crossover::Mode _CrossoverMode = crossover::Mode::LinkwitzRiley4;

    double _LowBand    = 0.; // Hz
    double _HighBand   = 0.; // Hz
    double _SampleRate = 0.; // Hz

    static constexpr double _BandPowerThreshold = 1e-9; // -90 dB
//  static constexpr double _BandPowerThreshold = 1e-8; // -80 dB
//  static constexpr double _BandPowerThreshold = 1e-6; // -60 dB

    double _LowOpacity  = 0.036;
    double _MidOpacity  = 0.018;
    double _HighOpacity = 0.013;

    D2D1_COLOR_F _MonoColor = D2D1::ColorF( .10f, 1.00f,  .45f); // Green

    D2D1_COLOR_F _LowColor  = D2D1::ColorF(1.00f,  .12f,  .04f); // Red
    D2D1_COLOR_F _MidColor  = D2D1::ColorF( .08f, 1.00f,  .25f); // Green
    D2D1_COLOR_F _HighColor = D2D1::ColorF( .08f,  .35f, 1.00f); // Blue

    crossover_filter_t _CrossoverL;
    crossover_filter_t _CrossoverR;

    double _Correlation = 0.f;
};
