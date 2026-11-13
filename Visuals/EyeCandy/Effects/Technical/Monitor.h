
/** $VER: Monitor.h (2026.11.12) **/

#pragma once

#include "Effect.h"

#include <array>
#include <string>
#include <vector>

#include <d2d1.h>
#include <dwrite.h>

#include <wrl/client.h>

namespace Monitor
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        using channel_pair_t = std::array<size_t, 2>;

        void Reset() noexcept override final;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & selectedChannels) noexcept;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t HistoryCount = 240; // 240 entries / 30 Hz = 8 seconds of history

        struct history_t
        {
            std::array<double, HistoryCount> Values {};
            size_t Cursor = 0;
            size_t Size = 0;
        };

        enum class hit_type_t
        {
            None,
            Bass,
            Drum
        };

        struct gauge_t
        {
            const wchar_t * Label = L"";
            const wchar_t * Unit = L"";
            double          Value   = 0.;
            double          Minimum = 0.;
            double          Maximum = 1.;
            unsigned int    Decimals = 1;
            D2D1_COLOR_F    Color {};

            double          PeakHoldValue   = 0.;
            double          PeakHoldElapsed = 0.;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels) noexcept;
        void AnalyzeSpectrum(const std::span<double> & samples) noexcept;
        void Update(double deltaTime) noexcept;
        void RecordHistory(double deltaTime) noexcept;

        void RenderGauges(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept;
        void RenderGauge(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const gauge_t & gauge) noexcept;
        void RenderHorizontalGauge(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const gauge_t & gauge) noexcept;
        void RenderHistogram(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept;
        void RenderHistory(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept;
        void RenderHistoryGraph(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds, const wchar_t * label, const history_t & history, double minimum, double maximum, const D2D1_COLOR_F & color, hit_type_t hitType = hit_type_t::None) noexcept;

        void DrawPanel(ID2D1RenderTarget * renderTarget, const D2D1_RECT_F & bounds) noexcept;
        void DrawText(ID2D1RenderTarget * renderTarget, const std::wstring & text, const D2D1_RECT_F & bounds, IDWriteTextFormat * format, const D2D1_COLOR_F & color) noexcept;
        void SetBrush(const D2D1_COLOR_F & color) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        HRESULT CreateDeviceIndependentResources() noexcept;
        void DeleteDeviceDependentResources() noexcept;

        static constexpr double MinDecibels = -60.;
        static constexpr double MaxDecibels =   0.;

        static double AmplitudeToDecibels(double amplitude) noexcept
        {
            return std::clamp(20. * std::log10(std::max(amplitude, 0.001)), MinDecibels, MaxDecibels);
        }

        void CalculateBandEdges(double minFrequency, double maxFrequency, double sampleRate) noexcept;
        void CalculateBandLabels();

    private:
        static constexpr size_t MonitorBandCount = 16;
        static constexpr double InverseBandCount = 1. / (MonitorBandCount - 1);

        static_assert(MonitorBandCount > 1);

        std::array<band_t, MonitorBandCount> _MonitorBands {};

        // Non-standard indicators
        double _MonoCompatibility  = 0.; // [  0, 1], A low result indicates that much of the signal exists as channel difference and may weaken in mono.
        double _ClippedFramesRatio = 0.; // [  0, 1], Percentage of frames that are clipped in the latest chunk.
        double _DCOffset           = 0.; // [-10, 10] in mFS (Thousandths of full scale). Health indicator.

        // Window function
        std::vector<double> _Window;
        double _WindowGain = 1.;

        // Bass detector
        double _BassDetectorSensitivity = 1.;   // [0.25, 4.0]
        double _BassDetector = 0.;
        bool   _BassHitEvent = false;
        double _BassHitIntensity = 0.;

        // Drum detector
        double _DrumDetectorSensitivity = 1.;   // [0.25, 4.0]
        double _DrumDetector = 0.;
        bool   _DrumHitEvent = false;
        double _DrumHitIntensity = 0.;
        double _DrumCooldown = 0.;

        double _TransientRate = 0.;             // Hits/s

    private:
        const D2D1_COLOR_F CanvasColor     = D2D1::ColorF(0.032f, 0.036f, 0.041f, 1.f);
        const D2D1_COLOR_F PanelColor      = D2D1::ColorF(0.052f, 0.058f, 0.064f, 0.98f);
        const D2D1_COLOR_F InstrumentColor = D2D1::ColorF(0.070f, 0.077f, 0.085f, 1.f);
        const D2D1_COLOR_F TrackColor      = D2D1::ColorF(0.140f, 0.152f, 0.164f, 1.f);
        const D2D1_COLOR_F BorderColor     = D2D1::ColorF(0.260f, 0.280f, 0.300f, 0.88f);
        const D2D1_COLOR_F GridColor       = D2D1::ColorF(0.300f, 0.320f, 0.340f, 0.30f);
        const D2D1_COLOR_F LabelColor      = D2D1::ColorF(0.730f, 0.750f, 0.770f, 1.f);
        const D2D1_COLOR_F MutedTextColor  = D2D1::ColorF(0.560f, 0.580f, 0.600f, 1.f);
        const D2D1_COLOR_F DataColor       = D2D1::ColorF(0.900f, 0.915f, 0.930f, 1.f);
        const D2D1_COLOR_F CautionColor    = D2D1::ColorF(0.950f, 0.680f, 0.150f, 1.f);
        const D2D1_COLOR_F CriticalColor   = D2D1::ColorF(0.930f, 0.210f, 0.170f, 1.f);
        const D2D1_COLOR_F BassEventColor  = D2D1::ColorF(0.950f, 0.580f, 0.130f, 1.f);
        const D2D1_COLOR_F DrumEventColor  = D2D1::ColorF(0.920f, 0.250f, 0.190f, 1.f);

        static constexpr size_t GaugeCount = 17;

        std::array<gauge_t, 15> Gauges =
        { {
            { L"Peak",              L"dBFS",        AmplitudeToDecibels(_SmoothedPeak), -60.,   0., 1, DataColor },
            { L"RMS",               L"dBFS",        AmplitudeToDecibels(_SmoothedRMS),  -60.,   0., 1, DataColor },

            { L"Crest Factor",      L"dB",          _SmoothedCrestFactor,                 0.,  30., 1, DataColor },
            { L"Stereo Width",      L"%",           _SmoothedStereoWidth * 100.,          0., 100., 0, DataColor },
            { L"Phase Energy",      L"norm",        _SmoothedPhaseEnergy,                 0.,   1., 2, DataColor },
            { L"Spectral Flux",     L"norm",        _SmoothedSpectralFlux,                0.,   1., 2, DataColor },

            { L"Bass Hit",          L"%",           _BassDetector * 100.,                 0.,   1., 0, DataColor },
            { L"Drum Hit",          L"%",           _DrumDetector * 100.,                 0.,   1., 0, DataColor },

            { L"Spectral Centroid", L"Hz",          _SpectralCentroid,    LoFrequency, HiFrequency, 0, DataColor },
            { L"Spectral Flatness", L"norm",        _SpectralFlatness,                    0.,   1., 2, DataColor },

            { L"Mono Compat.",      L"%",           _MonoCompatibility * 100.,            0., 100., 1, DataColor },
            { L"Clipped Frames",    L"%",           _ClippedFramesRatio * 100.,           0., 100., 2, DataColor },
            { L"DC Offset",         L"mFS",         _DCOffset,                          -10.,  10., 2, DataColor },

            { L"Bass Correlation",  L"r",           _BassCorrelation,                    -1.,   1., 2, DataColor },
            { L"Transient Rate",    L"hits/s",      _TransientRate,                       0.,  12., 1, DataColor }
        } };

        std::array<gauge_t, 2> HorizontalGauges =
        { {
            { L"L/R Balance",       L"dB",          _ChannelBalance,                    -18.,  18., 1, DataColor },
            { L"Correlation",       L"r",           _SmoothedCorrelation,                -1.,   1., 2, DataColor }
        } };

    private:
        static constexpr size_t SpectrumBandCount = MonitorBandCount;

        std::array<double, SpectrumBandCount + 1> _BandEdges;
        std::array<std::wstring, MonitorBandCount> _BandLabels;

    private:
        history_t _PeakHistory;
        history_t _RMSHistory;

        history_t _CorrelationHistory;
        history_t _FluxHistory;

        history_t _BassHitHistory;
        history_t _DrumHitHistory;

        history_t _BassLevelHistory;
        history_t _MidLevelHistory;
        history_t _TrebleLevelHistory;

        double _HistoryElapsed = 0.;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<IDWriteFactory> _WriteFactory;

        ComPtr<IDWriteTextFormat> _LabelFormat;
        ComPtr<IDWriteTextFormat> _ValueFormat;
        ComPtr<IDWriteTextFormat> _SmallFormat;
    };
}
