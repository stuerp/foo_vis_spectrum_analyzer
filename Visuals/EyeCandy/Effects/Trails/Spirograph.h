
/** $VER: Spirograph.h (2026.10.22) */

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Spirograph
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
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & channels) noexcept;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t BandCount = 12;
        static constexpr size_t CurveCount = 7;

        struct band_t
        {
            double Energy         = 0.; // Spectrum bands
            double SmoothedEnergy = 0.;
        };

        struct curve_t
        {
            std::vector<D2D1_POINT_2F> Points;
            D2D1_COLOR_F Color {};
            float Width     = 1.f;
            float Intensity = 0.f;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels);
        void Update(double deltaTime);

        void GenerateCurves() noexcept;

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;

        void RenderCurves(ID2D1RenderTarget * renderTarget) noexcept;
        void DrawSmoothCurve(ID2D1RenderTarget * renderTarget, const curve_t & curve) noexcept;

        void SetBrush(const D2D1_COLOR_F & color, float opacity) noexcept;

        D2D1_COLOR_F Colorize(double value) const noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::array<band_t, BandCount> _Bands {};
        std::array<double, 12> _PitchClasses {};

        std::vector<curve_t> _Curves;

        double _SampleRate       = 44100.;

        double _Rhythm           = 0.;  // Rhythm estimate
        double _SmoothedRhythm   = 0.;

        double _RhythmPulse      = 0.;

        double _Tonality         = 0.; // Dominant pitch
        double _SmoothedTonality = 0.;

        double _Rotation         = 0.;

        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
