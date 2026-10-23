
/** $VER: ParticleVortex.h (2026.10.18) **/

#pragma once

#include "Effect.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include <d2d1.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ParticleVortex
{
    using namespace EyeCandy;

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
        static constexpr size_t BytesPerPixel = sizeof(uint32_t);

        struct particle_t
        {
            D2D1_POINT_2F Position {};
            D2D1_POINT_2F Previous {};
            D2D1_POINT_2F Velocity {};

            double Age         = 0.;
            double Lifetime    = 1.;

            double OrbitOffset = 0.;
            double RadiusBias  = 1.;
            double Brightness  = 1.;

            size_t Band   = 0;
            uint32_t Seed = 0;
        };

        struct center_t
        {
            D2D1_POINT_2F Position {};
            D2D1_POINT_2F Previous {};

            double Energy    = 0.;
            double Peak      = 0.;
            double Frequency = 0.;
            double Phase     = 0.;
        };

    private:
        void AnalyzeAudio(const audio_chunk_impl & chunk);
        void Update(double deltaTime);

        void UpdateCenters(double deltaTime);

        void GenerateWork(double deltaTime);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void UpdateParticle(size_t index, double deltaTime) noexcept;
        void RespawnParticle(particle_t & particle, size_t index) noexcept;

        void Fade(ID2D1RenderTarget * renderTarget, double deltaTime) noexcept;
        void RenderVortex(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderCenterGlow(ID2D1RenderTarget * renderTarget, const center_t & center, size_t band) noexcept;
        void RenderIdleVortex(ID2D1RenderTarget * renderTarget) noexcept;

        void DrawGlowLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, float width) noexcept;
        void DrawLine(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F from, D2D1_POINT_2F to, uint32_t color, float intensity, int radius) noexcept;

        void DrawGlowPoint(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, uint32_t color, float intensity, float radius) noexcept;

        uint32_t Colorize(size_t band, double energy, double particlePhase) const noexcept;

        void SetBrush(uint32_t color, float opacity) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<particle_t> _Particles;

        std::atomic<size_t> _NextParticle { 0 };
        double _ParticleDeltaTime = 0.;

        std::array<center_t, FrequencyBandCount> _Centers {};

        std::array<double, FrequencyBandCount> _BandEnergy {};
        std::array<double, FrequencyBandCount> _SmoothedBandEnergy {};
        std::array<double, FrequencyBandCount> _PreviousBandEnergy {};

        double _SmoothedPeak = 0.0f;
        double _SmoothedRMS = 0.0f;

        double _BassEnergy = 0.0f;
        double _MidEnergy = 0.0f;
        double _TrebleEnergy = 0.0f;

        double _SpectralFlux = 0.0f;
        double _SmoothedFlux = 0.0f;

    private:
        static constexpr double TransitionInterval = 3.; // seconds

        double _TransitionTime = 1.;

        double _NextTransitionTime = TransitionInterval;
        size_t _NextThemeIndex = 0;

    private:
        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
