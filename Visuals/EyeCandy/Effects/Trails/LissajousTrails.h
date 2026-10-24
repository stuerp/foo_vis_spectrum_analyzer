
/** $VER: LissajousTrails.h (2026.10.23) **/

#pragma once

#include "Effect.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>

#include <wrl/client.h>

namespace LissajousTrails
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        using channel_pair_t = std::array<size_t, 2>;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk, const channel_pair_t & channels) noexcept;

        std::wstring GetStatus() noexcept override final;

    private:
        struct stereo_sample_t
        {
            double Left  = 0.;
            double Right = 0.;
        };

        struct segment_t
        {
            D2D1_POINT_2F From {};
            D2D1_POINT_2F To {};
            uint32_t Color   = 0;
            double Intensity = 0.;
        };

        struct field_t
        {
            double Rotation    = 0.;
            double Energy      = 0.;
            double PhaseAmount = 0.;
            double Time        = 0.;
        };

        struct theme_t
        {
            double HueStart       = 0.;
            double HueRange       = 0.;
            double AntiPhaseShift = 0.;
            double Saturation     = 0.;
            double AntiPhaseBoost = 0.;
            D2D1_COLOR_F Background {};
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels);
        void Update(double deltaTime);

        void GenerateWork() noexcept;
        void GenerateSegment(size_t index, const field_t & field) noexcept;

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;
        void RenderCurve(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderIdleCurve(ID2D1RenderTarget * renderTarget) noexcept;

        void DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity) noexcept;
        void DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, int radius) noexcept;
        void SetBrush(uint32_t color, float opacity) noexcept;

        D2D1_POINT_2F TransformSample(stereo_sample_t sample, double stereoRotation) const noexcept;
        
        uint32_t Colorize(double value) const noexcept;
        void GetThemeTransition(size_t & from, size_t & to, double & amount) const noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void GenerateSpline() noexcept;
        static D2D1_POINT_2F EvaluateSpline(D2D1_POINT_2F p0, D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3, double amount) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<stereo_sample_t> _Samples;
        std::vector<segment_t> _Segments;
        std::vector<segment_t> _SplineSegments;

        field_t _Field {};
        std::atomic<size_t> _NextSegment { 0 };

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
