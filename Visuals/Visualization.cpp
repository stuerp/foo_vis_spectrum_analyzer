
/** $VER: Visualization.cpp (2026.10.28) P. Stuer - Base class for all visualization elements. **/

#include "pch.h"

#include "Visualization.h"

#include "Spectrum.h"
#include "Spectrogram.h"
#include "PeakMeter.h"
#include "BalanceMeter.h"
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
std::unique_ptr<visualization_t> visualization_t::Create(state_t & state) noexcept
{
    switch (state._VisualizationType)
    {
        default:

        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
            return std::make_unique<Spectrum::spectrum_t>(state);

        case VisualizationType::Spectrogram:
            return std::make_unique<Spectrum::spectrogram_t>(state);

        case VisualizationType::PeakMeter:
            return std::make_unique<Metering::peak_meter_t>(state);

        case VisualizationType::BalanceMeter:
            return std::make_unique<Metering::balance_meter_t>(state);

        case VisualizationType::Oscilloscope:
            if (!state._XYMode)
                return std::make_unique<Oscilloscope::oscilloscope_t>(state);

            return std::make_unique<Oscilloscope::oscilloscope_xy_t>(state);

        case VisualizationType::BitMeter:
            return std::make_unique<BitMeter::bit_meter_t>(state);

        case VisualizationType::Goniometer:
            return std::make_unique<Goniometer::goniometer_t>(state);

        case VisualizationType::EyeCandy:
            return std::make_unique<EyeCandy::eye_candy_t>(state);

        case VisualizationType::Tester:
            return std::make_unique<Test::tester_t>(state);

        case VisualizationType::D3DTester:
            return std::make_unique<Test::d3d_tester_t>(state);
    }
}
