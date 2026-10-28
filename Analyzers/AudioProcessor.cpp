
/** $VER: AudioProcessor.cpp (2026.10.28) P. Stuer **/

#include "pch.h"

#include "AudioProcessor.h"

#include "BitmeterProcessor.h"
#include "EyeCandyProcessor.h"
#include "GoniometerProcessor.h"
#include "MeteringProcessor.h"
#include "OscilloscopeProcessor.h"
#include "SpectrumProcessor.h"
#include "TestProcessor.h"

#include "Support.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#pragma hdrstop

/// <summary>
/// Creates an instance of the specified processor.
/// </summary>
std::unique_ptr<audio_processor_t> audio_processor_t::Create(state_t & state) noexcept
{
    switch (state._VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
        case VisualizationType::Spectrogram:
            return std::make_unique<Spectrum::processor_t>(state);

        case VisualizationType::PeakMeter:
        case VisualizationType::BalanceMeter:
            return std::make_unique<Metering::processor_t>(state);

        case VisualizationType::Oscilloscope:
            return std::make_unique<Oscilloscope::processor_t>(state);

        case VisualizationType::BitMeter:
            return std::make_unique<BitMeter::processor_t>(state);

        case VisualizationType::Goniometer:
            return std::make_unique<Goniometer::processor_t>(state);

        case VisualizationType::EyeCandy:
            return std::make_unique<EyeCandy::processor_t>(state);

        case VisualizationType::Tester:
        case VisualizationType::D3DTester:
            return std::make_unique<Test::processor_t>(state);
    }
}

/// <summary>
/// Configures this instance.
/// </summary>
void audio_processor_t::Configure(const graph_options_t * options) noexcept
{
    _Options = options;

    Reset();
}

/// <summary>
/// Resets this instance.
/// </summary>
void audio_processor_t::Reset() noexcept
{
    _SampleRate    = 0;
    _ChannelCount  = 0;
    _ChannelConfig = 0;

    _Chrono.Reset();
}

/// <summary>
/// Processes an audio chunk.
/// </summary>
void audio_processor_t::Process(const audio_chunk & chunk) noexcept
{
}

/// <summary>
/// 
/// </summary>
void audio_processor_t::UpdatePeaks(bool isStopped) noexcept
{
}

const uint32_t audio_processor_t::ChannelPairs[6] =
{
    (uint32_t) Channels::FrontLeft       | (uint32_t) Channels::FrontRight,
    (uint32_t) Channels::BackLeft        | (uint32_t) Channels::BackRight,

    (uint32_t) Channels::FrontCenterLeft | (uint32_t) Channels::FrontCenterRight,
    (uint32_t) Channels::SideLeft        | (uint32_t) Channels::SideRight,

    (uint32_t) Channels::TopFrontLeft    | (uint32_t) Channels::TopFrontRight,
    (uint32_t) Channels::TopBackLeft     | (uint32_t) Channels::TopBackRight,
};
