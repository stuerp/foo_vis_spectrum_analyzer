
/** $VER: RibbonFlow.h (2026.10.22) */

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace RibbonFlow
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
        static constexpr size_t BandCount = 10;

        struct band_t
        {
            double Energy         = 0.;
            double SmoothedEnergy = 0.;
            double Phase          = 0.;
        };

        struct ribbon_t
        {
            std::vector<D2D1_POINT_2F> Points;
            uint32_t Color  = 0;
            float Width     = 1.f;
            float Intensity = 0.f;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk, const channel_pair_t & channels);
        void Update(double deltaTime);
        void GenerateRibbons() noexcept;
        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;
        void RenderRibbons(ID2D1RenderTarget * renderTarget) noexcept;
        void DrawSmoothRibbon(ID2D1RenderTarget * renderTarget, const ribbon_t & ribbon) noexcept;
        void SetBrush(uint32_t color, float opacity) noexcept;

        uint32_t Colorize(double value) const noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::array<band_t, BandCount> _Bands {};
        std::vector<ribbon_t> _Ribbons;

        double _SampleRate = 44100.;

        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;
        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
