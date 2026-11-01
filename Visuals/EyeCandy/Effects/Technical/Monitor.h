
/** $VER: Monitor.h (2026.10.25) */

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

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & selectedChannels) noexcept;

        void Reset() noexcept override final;
        std::wstring GetStatus() noexcept override final;

        void SetBassDetectorSensitivity(double value) noexcept
        {
            _BassDetectorSensitivity = std::clamp(value, 0.25, 4.0);
        }

        double GetBassDetectorSensitivity() const noexcept
        {
            return _BassDetectorSensitivity;
        }

        void SetDrumDetectorSensitivity(double value) noexcept
        {
            _DrumDetectorSensitivity = std::clamp(value, 0.25, 4.0);
        }

        double GetDrumDetectorSensitivity() const noexcept
        {
            return _DrumDetectorSensitivity;
        }

    private:
        static constexpr size_t HistoryCount = 240;
        static constexpr size_t MonitorBandCount = 16;

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
            double Value   = 0.;
            double Minimum = 0.;
            double Maximum = 1.;
            unsigned int Decimals = 1;
            double PeakHoldValue = 0.;
            D2D1_COLOR_F Color {};
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels) noexcept;
        void AnalyzeSpectrum(const std::vector<double> & samples, double sampleRate) noexcept;
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

    private:
        std::array<double, MonitorBandCount> _MonitorBands{ };
        std::array<double, MonitorBandCount> _SmoothedMonitorBands{ };
        std::array<double, MonitorBandCount> _PreviousBands{ };

        double _CrestFactor = 0.;

        double _SmoothedCrestFactor = 0.;

        double _HistoryElapsed = 0.;
        double _SampleRate = 44100.;
        double _SmoothedPhaseEnergy = 0.;
        double _BassDetector = 0.;
        double _DrumDetector = 0.;
        double _DrumCooldown = 0.;
        double _PreviousRMS = 0.;

        double _BassDetectorSensitivity = 1.;   // [0.25, 4.0]
        double _DrumDetectorSensitivity = 1.;   // [0.25, 4.0]

        bool _BassHitEvent = false;
        bool _DrumHitEvent = false;
        double _BassHitIntensity = 0.;
        double _DrumHitIntensity = 0.;

        double _ChannelBalance = 0.;
        double _SpectralCentroid = 0.;
        double _SpectralFlatness = 0.;
        double _MonoCompatibility = 1.;
        double _ClippedSampleRatio = 0.;
        double _DCOffset = 0.;
        double _BassCorrelation = 1.;
        double _TransientRate = 0.;

        std::vector<double> _AnalysisSamples;
        std::vector<double> _AnalysisWindow;
        double _AnalysisWindowGain = 1.;

        std::array<double, 17> _GaugePeakHold{ };
        std::array<double, 17> _GaugePeakHoldElapsed{ };

    private:
        history_t _PeakHistory;
        history_t _RMSHistory;
        history_t _CorrelationHistory;
        history_t _FluxHistory;
        history_t _BassHitHistory;
        history_t _DrumHitHistory;
        history_t _BassEnergyHistory;
        history_t _MidEnergyHistory;
        history_t _TrebleEnergyHistory;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<IDWriteFactory> _WriteFactory;
        ComPtr<IDWriteTextFormat> _TitleFormat;
        ComPtr<IDWriteTextFormat> _LabelFormat;
        ComPtr<IDWriteTextFormat> _ValueFormat;
        ComPtr<IDWriteTextFormat> _SmallFormat;
    };
}
