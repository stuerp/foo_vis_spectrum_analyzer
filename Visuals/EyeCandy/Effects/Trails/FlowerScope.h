
/** $VER: FlowerScope.h (2026.11.02) **/

#pragma once

#include "Effect.h"

#include <array>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace FlowerScope
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        using channel_pair_t = std::array<size_t, 2>;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & channels) noexcept;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t BandCount        = 12;
        static constexpr size_t CurveCount       =  5;
        static constexpr size_t PaletteCount     = 12;
        static constexpr size_t FlowerCount      = 18;
        static constexpr size_t RenderCurveCount = CurveCount * 2;

        struct band_t
        {
            double Energy = 0.;
            double SmoothedEnergy = 0.;
        };

        struct curve_t
        {
            std::vector<D2D1_POINT_2F> Points;
            D2D1_COLOR_F Color{ };

            float Width = 1.f;
            float Intensity = 0.f;
            float OuterGlow = 10.f;
            float InnerGlow = 4.f;
            float OuterOpacity = .07f;
            float InnerOpacity = .22f;
            float CoreOpacity = 1.f;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels);
        void Update(double deltaTime);

        void GenerateCurves() noexcept;
        void GenerateCurve(size_t) noexcept;
        double FlowerPattern(size_t, double, double, double, double) const noexcept;
        size_t FlowerPetalOffset(size_t) const noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker(size_t) noexcept;

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;
        void RenderCurves(ID2D1RenderTarget * renderTarget) noexcept;
        void DrawSmoothCurve(ID2D1RenderTarget * renderTarget, const curve_t & curve) noexcept;
        void SetBrush(const D2D1_COLOR_F & color, float opacity) noexcept;

        D2D1_COLOR_F Colorize(size_t palette, double value) const noexcept;

    private:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::array<band_t, BandCount> _Bands{ };
        std::vector<double> _LeftSamples;
        std::vector<double> _RightSamples;

    private:
        std::vector<curve_t> _Curves;

        double _SampleRate = 44100.;
        double _Rhythm = 0.;
        double _SmoothedRhythm = 0.;
        double _RhythmPulse = 0.;
        double _HarmonicCenter = 0.;
        double _SmoothedHarmonic = 0.;
        double _StereoCorrelation = 1.;
        double _SmoothedCorrelation = 1.;
        double _StereoPhase = 0.;
        double _SmoothedPhase = 0.;
        double _Rotation = 0.;
        double _BassGlow = 0.;
        size_t _CurvePointCount = 640;
        size_t _CurveBasePetals = 2;
        double _CurveExtent = 0.;
        double _CurveStereoWidth = 0.;
        double _FlowerCenterX = 0.;
        double _FlowerCenterY = 0.;
        double _PaletteTime = 0.;
        double _FlowerTime = 0.;
        size_t _FlowerA = 0;
        size_t _FlowerB = 1;
        double _FlowerBlend = 0.;
        size_t _PaletteA = 0;
        size_t _PaletteB = 1;
        double _PaletteBlend = 0.;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;
        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
