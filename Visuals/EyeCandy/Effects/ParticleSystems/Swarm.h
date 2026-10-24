
/** $VER: Swarm.h (2026.10.23) */

#pragma once

#include "Effect.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace Swarm
{
    using Microsoft::WRL::ComPtr;

    using namespace EyeCandy;

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() override = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget *, double, audio_chunk &) noexcept override;
        void Reset() noexcept override;
        std::wstring GetStatus() noexcept override;

    private:
        static constexpr size_t BandCount = 12;
        static constexpr size_t NeighborSamples = 24;

        struct agent_t
        {
            D2D1_POINT_2F Position {};
            D2D1_POINT_2F Previous {};
            D2D1_POINT_2F Velocity {};
            size_t Band   = 0;
            uint32_t Seed = 0;
            double Phase      = 0.;
            double Size       = 1.;
            double Brightness = 1.;
        };

        void AnalyzeAudio(const audio_chunk & chunk);

        void Update(double) noexcept;
        void UpdateAgent(size_t, double) noexcept;
        void RespawnAgent(agent_t &, size_t) noexcept;

        void DrawBackgroundGradient(ID2D1RenderTarget *, double) noexcept;

        void Fade(ID2D1RenderTarget *, double) noexcept;
        void RenderSwarm(ID2D1RenderTarget *) noexcept;
        void RenderIdleSwarm(ID2D1RenderTarget *) noexcept;

        void DrawNeonLine (ID2D1RenderTarget *, D2D1_POINT_2F, D2D1_POINT_2F, const D2D1_COLOR_F & color, float, float) noexcept;
        void DrawNeonPoint(ID2D1RenderTarget *, D2D1_POINT_2F,                const D2D1_COLOR_F & color, float, float) noexcept;
        void DrawLine     (ID2D1RenderTarget *, D2D1_POINT_2F, D2D1_POINT_2F, const D2D1_COLOR_F & color, float, float) noexcept;

        D2D1_COLOR_F Colorize(size_t, double, double) const noexcept;
        double GetPaletteHue(double, double) const noexcept;
        void SetBrush(const D2D1_COLOR_F & color, float opacity, float gain = 1.f) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget *) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<agent_t> _Agents;
        std::vector<agent_t> _Snapshot;

        std::array<double, BandCount> _BandEnergy {};
        std::array<double, BandCount> _SmoothedBandEnergy {};
        std::array<double, BandCount> _PreviousBandEnergy {};

        double _StereoBalance         = 0.;
        double _SmoothedStereoBalance = 0.;

        double _PreviousBeatPulse     = 0.;
        double _BeatColorPhase        = 0.;
        double _GradientPhase         = 0.;
        double _PaletteBlend          = 1.;
        double _BeatCooldown          = 0.;

        size_t _PaletteIndex          = 0;
        size_t _PreviousPaletteIndex  = 0;

        ComPtr<ID2D1SolidColorBrush> _Brush;
        ComPtr<ID2D1StrokeStyle> _StrokeStyle;

        ComPtr<ID2D1BitmapRenderTarget> _BackBufferRenderTarget;
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
