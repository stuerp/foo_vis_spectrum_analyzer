
/** $VER: VectorGrid.h (2026.10.23) */

#pragma once

#include "Effect.h"

#include <algorithm>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace VectorGrid
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Reset() noexcept override final;
        std::wstring GetStatus() noexcept override final;

        void SetBassResponse(double value) noexcept { _BassResponse = std::clamp(value, 0., 3.); }
        double GetBassResponse() const noexcept { return _BassResponse; }

        void SetBeatResponse(double value) noexcept { _BeatResponse = std::clamp(value, 0., 3.); }
        double GetBeatResponse() const noexcept { return _BeatResponse; }

        void SetMidResponse(double value) noexcept { _MidResponse = std::clamp(value, 0., 3.); }
        double GetMidResponse() const noexcept { return _MidResponse; }

        void SetTrebleResponse(double value) noexcept { _TrebleResponse = std::clamp(value, 0., 3.); }
        double GetTrebleResponse() const noexcept { return _TrebleResponse; }

        void SetTheme(ColorTheme theme) noexcept { SetColorTheme(theme); }
        ColorTheme GetTheme() const noexcept { return GetColorTheme(); }
        void NextTheme() noexcept { NextColorTheme(); }
        void PreviousTheme() noexcept { PreviousColorTheme(); }

    private:
        static constexpr size_t MaximumFrameCount = 2048;
        static constexpr size_t MaximumColumns = 49;
        static constexpr size_t MaximumRows = 72;

        struct grid_sample_t
        {
            D2D1_POINT_2F Position {};
            double ViewDepth = 0.;
            double Depth     = 0.;
            double Height    = 0.;
            bool Projectable = false;
        };

        struct palette_t
        {
            D2D1_COLOR_F BackgroundTop {};
            D2D1_COLOR_F BackgroundBottom {};
            D2D1_COLOR_F Horizon {};
            D2D1_COLOR_F Longitudinal {};
            D2D1_COLOR_F Lateral {};
            D2D1_COLOR_F Accent {};
            D2D1_COLOR_F Sun {};
        };

        void AnalyzeAudio(const audio_chunk & chunk);
        void Update(double deltaTime) noexcept;
        void UpdatePalette() noexcept;
        void BuildGrid();

        void StartWorkers();
        void StopWorkers() noexcept;
        void GenerateWork(size_t workerIndex, size_t workerCount) noexcept;

        grid_sample_t ProjectGridPoint(double x, double z) const noexcept;
        double EvaluateWave(double x, double z) const noexcept;

        void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept override final;
        void RenderSun(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderGrid(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderHorizon(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderBeatFlash(ID2D1RenderTarget * renderTarget) noexcept;

        void DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, D2D1_COLOR_F color, float intensity, float strokeWidth) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        palette_t _Palette {};

        size_t _ColumnCount = 33;
        size_t _RowCount    = 52;
        size_t _CellCount   =  0;

        std::vector<grid_sample_t> _Grid;

        double _GridRowStep      = 0.;
        double _GridTravelOffset = 0.;

        double _Travel        = 0.; // Travel distance of the grid per frame

        double _WaveEnvelope  = 0.;
        double _MidEnvelope   = 0.;
        double _TrebleImpulse = 0.;

        double _PreviousMidEnvelope    = 0.;
        double _PreviousTrebleEnvelope = 0.;

        double _CameraKick      = 0.;
        double _HorizonPulse    = 0.;
        double _BeatDeformation = 0.;
        double _BeatPhase       = 0.;

        double _BassResponse   = 1.65;
        double _BeatResponse   = 1.45;
        double _MidResponse    = 1.55;
        double _TrebleResponse = 1.75;

        ComPtr<ID2D1SolidColorBrush> _GlowBrush;
        ComPtr<ID2D1SolidColorBrush> _CoreLineBrush;
        ComPtr<ID2D1SolidColorBrush> _HorizonBrush;
        ComPtr<ID2D1GradientStopCollection> _SkyStops;
        ComPtr<ID2D1LinearGradientBrush> _SkyBrush;
        ComPtr<ID2D1GradientStopCollection> _SunStops;
        ComPtr<ID2D1RadialGradientBrush> _SunBrush;
        ComPtr<ID2D1GradientStopCollection> _FlashStops;
        ComPtr<ID2D1RadialGradientBrush> _FlashBrush;
        ComPtr<ID2D1StrokeStyle> _RoundStrokeStyle;
    };
}
