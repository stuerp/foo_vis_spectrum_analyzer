
/** $VER: Visualization.cpp (2026.10.11) P. Stuer - Base class for all visualization elements. **/

#include "pch.h"

#include "Visualization.h"

#include "Spectrum.h"
#include "Spectrogram.h"
#include "PeakMeter.h"
#include "LevelMeter.h"
#include "Oscilloscope.h"
#include "OscilloscopeXY.h"
#include "BitMeter.h"
#include "Goniometer.h"
#include "EyeCandy.h"

#include "Tester.h"
#include "D3DTester.h"

/// <summary>
/// Creates an instance of the specified visualization.
/// </summary>
std::unique_ptr<visualization_t> visualization_t::Create(const state_t & state) noexcept
{
    switch (state._VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
            return std::make_unique<spectrum_t>();

        case VisualizationType::Spectrogram:
            return std::make_unique<spectrogram_t>();

        case VisualizationType::PeakMeter:
            return std::make_unique<peak_meter_t>();

        case VisualizationType::LevelMeter:
            return std::make_unique<level_meter_t>();

        case VisualizationType::Oscilloscope:
            if (!state._XYMode)
                return std::make_unique<oscilloscope_t>();

            return std::make_unique<oscilloscope_xy_t>();

        case VisualizationType::BitMeter:
            return std::make_unique<bit_meter_t>();

        case VisualizationType::Goniometer:
            return std::make_unique<goniometer_t>();

        case VisualizationType::EyeCandy:
            return std::make_unique<eye_candy_t>();

        case VisualizationType::Tester:
            return std::make_unique<tester_t>();

        case VisualizationType::D3DTester:
            return std::make_unique<d3d_tester_t>();
    }
}
