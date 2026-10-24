
/** $VER: DynamicSystemEffect.h (2026.10.20) */

#pragma once

#include "Effect.h"
#include "ThreeD.h"

#include <cstdint>

#include <d2d1.h>
#include <wrl/client.h>

namespace DynamicSystem
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t : public EyeCandy::effect_t, public EyeCandy::ThreeD
    {
    public:
        effect_t() = default;
        ~effect_t() =default;

        effect_t(const DynamicSystem::effect_t &) = delete;
        effect_t(DynamicSystem::effect_t && other) = delete;

        effect_t & operator=(const DynamicSystem::effect_t &) = delete;
        effect_t & operator=(DynamicSystem::effect_t && other) = delete;

    protected:
        virtual void Reset() noexcept override;

        void AnalyzeAudio(const audio_chunk & chunk);

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;

        void RenderAxes(ID2D1RenderTarget * renderTarget, double length) noexcept;
        void DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float thickness) noexcept;
        void DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float thickness) noexcept;

        void SetSplineSubdivision(size_t subdivision) noexcept;
        void SetSplineSmoothing(double smoothing) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

        void GenerateSpline() noexcept;
        static point3d_t EvaluateSpline(point3d_t p0, point3d_t p1, point3d_t p2, point3d_t p3, double amount) noexcept;

    protected:
        struct curve_theme_t : EyeCandy::theme_t
        {
            std::array<D2D1_COLOR_F, 3> Colors {};
        };

        const std::array<effect_t::curve_theme_t, 6> CurveThemes =
        {{
            { L"Aurora",      {{ D2D1::ColorF(0x35F2D0), D2D1::ColorF(0x4D7CFE), D2D1::ColorF(0xC45CFF) }} },
            { L"Solar Flare", {{ D2D1::ColorF(0xFFD166), D2D1::ColorF(0xFF6B35), D2D1::ColorF(0xEF476F) }} },
            { L"Deep Ocean",  {{ D2D1::ColorF(0x00E5FF), D2D1::ColorF(0x0077FF), D2D1::ColorF(0x3A0CA3) }} },
            { L"Orchid",      {{ D2D1::ColorF(0xFF8CC6), D2D1::ColorF(0xB86BFF), D2D1::ColorF(0x6247EA) }} },
            { L"Verdant",     {{ D2D1::ColorF(0xD9ED92), D2D1::ColorF(0x52B788), D2D1::ColorF(0x168AAD) }} },
            { L"Neon Night",  {{ D2D1::ColorF(0xF72585), D2D1::ColorF(0x7209B7), D2D1::ColorF(0x4CC9F0) }} }
        }};

        size_t _CurveThemeFrom = 0;
        size_t _CurveThemeTo   = 0;

        void UpdateCurveTheme(double deltaTime) noexcept;
        void BeginCurveThemeTransition(size_t themeIndex) noexcept;
        D2D1_COLOR_F SampleCurveTheme(const curve_theme_t & theme, double position) noexcept;
        uint32_t GetCurveColor(double position) noexcept;

        void SetBrush(uint32_t color, float opacity) noexcept
        {
            _Brush->SetColor(D2D1::ColorF(color));
            _Brush->SetOpacity(Clamp(opacity));
        }

        uint32_t Colorize(double value) const noexcept;

    protected:
        std::vector<point3d_t> _Points;
        std::vector<point3d_t> _SplinePoints;

        size_t _SplineSubdivision = 2;
        double _SplineSmoothing = 0.35;

    protected:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
