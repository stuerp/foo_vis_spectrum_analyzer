
/** $VER: Fireworks.h (2026.10.23) **/

#pragma once

#include "Effect.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Fireworks
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

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept override final;
        void Reset() noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t FrequencyBandCount = 12;

        static_assert(FrequencyBandCount > 1);

        static constexpr size_t FlareTrailPointCount = 24;
        static constexpr double FlareTrailLifetime   = 0.32;

        struct flare_trail_point_t
        {
            D2D1_POINT_2F Position {};
            double Age = FlareTrailLifetime;
        };

        struct flare_t
        {
            D2D1_POINT_2F Position {};
            D2D1_POINT_2F Previous {};
            D2D1_POINT_2F Velocity {};

            double Age    = 0.;
            double Fuse   = 1.;
            double Energy = 0.;
            double Hue    = 0.;
            size_t Band   = 0;
            uint32_t Seed = 0;

            std::array<flare_trail_point_t, FlareTrailPointCount> Trail {};
            size_t TrailCount = 0;

            bool IsActive = false;
        };

        struct spark_t
        {
            D2D1_POINT_2F Position {};
            D2D1_POINT_2F Previous {};
            D2D1_POINT_2F Velocity {};

            double Age        = 0.;
            double Lifetime   = 1.;
            double Hue        = 0.;
            double Saturation = 1.;
            double Brightness = 1.;
            double Drag       = 1.;
            double Gravity    = 1.;
            uint32_t Seed     = 0;

            bool IsActive = false;
        };

    private:
        void AnalyzeAudio(const audio_chunk_impl & chunk);
        void Update(double deltaTime);

        void LaunchFlares(double deltaTime);
        void UpdateFlares(double deltaTime);
        void GenerateWork(double deltaTime);
        void UpdateSpark(size_t index, double deltaTime) noexcept;
        spark_t * AcquireSpark() noexcept;

        void LaunchFlare(size_t band, double energy) noexcept;
        void ExplodeFlare(flare_t & flare) noexcept;

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;
        void RenderSky(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderFireworks(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderIdle(ID2D1RenderTarget * renderTarget) noexcept;

        void RenderFlareHeads(ID2D1RenderTarget * renderTarget) noexcept;

        void DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept;
        void DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, int radius) noexcept;
        void DrawGlowPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, uint32_t color, float intensity, float radius) noexcept;

        void UpdateFlareTrail(flare_t & flare, double deltaTime) noexcept;
        void RenderFlareTrail(ID2D1RenderTarget * renderTarget, const flare_t & flare) noexcept;

        void DrawForegroundLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float opacity, float width) noexcept;
        void DrawForegroundGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept;

        void SetBrush(ID2D1SolidColorBrush * brush, uint32_t color, float opacity) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<flare_t> _Flares;
        std::vector<spark_t> _Sparks;

        std::atomic<size_t> _NextSpark { 0 };
        double _SparkDeltaTime = 0.;

        std::array<double, FrequencyBandCount> _BandEnergy {};          // Energy at logarithmically spaced frequencies.
        std::array<double, FrequencyBandCount> _SmoothedBandEnergy {};
        std::array<double, FrequencyBandCount> _PreviousBandEnergy {};
        std::array<double, FrequencyBandCount> _BandCooldown {};

        double _SkyLight          = 0.;
        double _SkyLightTarget    = 0.;
        double _SkyHue            = 0.60;

        double _LaunchAccumulator = 0.;
        double _BassCooldown      = 0.;

        uint32_t _RandomState = 0xC001D00Du;
        uint32_t _LaunchSequence = 0;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;

        ComPtr<ID2D1SolidColorBrush> _ForegroundBrush;
    };
}
