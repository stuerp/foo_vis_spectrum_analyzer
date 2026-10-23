
/** $VER: NeonTunnel.h (2026.10.18) **/

#pragma once

#include "Effect.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace NeonTunnel
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

        void SetBassResponse(double value) noexcept
        {
            _BassResponse = std::clamp(value, 0., 3.);
        }

        double GetBeatResponse() const noexcept
        {
            return _BeatResponse;
        }

        void SetBeatResponse(double value) noexcept
        {
            _BeatResponse = std::clamp(value, 0., 3.);
        }

        double GetBassResponse() const noexcept
        {
            return _BassResponse;
        }

        std::wstring GetStatus() noexcept override final;

    private:
        static constexpr size_t MaximumTunnelSides = 24;
        static constexpr size_t GlowLayerCount = 3;

        static constexpr double RotationSwitchThreshold       = 0.64;
        static constexpr double RotationSwitchRelease         = 0.32;
        static constexpr double RotationSwitchCooldownSeconds = 0.75;

        struct ring_t
        {
            double Z           = 1.;
            double PreviousZ   = 1.;

            // Normalized physical depth. Equal phase intervals correspond to equal camera-space Z intervals.
            double Phase       = 0.;

            double Rotation    = 0.;
            double RadiusScale = 1.;
            double Brightness  = 1.;
            double HueOffset   = 0.;

            // Theme snapshot captured when this ring enters at the far plane. Successive rings capture successive transition values, producing a smooth color wave through the tunnel.
            ColorTheme ThemeFrom = ColorTheme::Synthwave;
            ColorTheme ThemeTo   = ColorTheme::Synthwave;

            double ThemeBlend = 1.;

            uint32_t Seed = 0;
        };

        struct projected_ring_t
        {
            std::array<D2D1_POINT_2F, MaximumTunnelSides> Points{};

            size_t SideCount     = 0;
            size_t SourceIndex   = 0;
            double Depth         = 0.;
            double Brightness    = 0.;
            double HueOffset     = 0.;

            ColorTheme ThemeFrom = ColorTheme::Synthwave;
            ColorTheme ThemeTo   = ColorTheme::Synthwave;

            double ThemeBlend = 1.;

            bool Visible = false;
        };

        void AnalyzeAudio(const audio_chunk_impl & chunk);
        void Update(double deltaTime);

        void GenerateWork() noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        projected_ring_t ProjectRing(const ring_t & ring, size_t ringIndex) const noexcept;
        D2D1_POINT_2F ProjectPoint(double angle, double z, double radiusScale) const noexcept;

        void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept override final;

        void RenderTunnel(ID2D1RenderTarget * renderTarget);
        void RenderCoreGlow(ID2D1RenderTarget * renderTarget) noexcept;
        void RenderBeatFlash(ID2D1RenderTarget * renderTarget) noexcept;

        void CreateRingGeometry(ID2D1Factory * factory, const std::vector<projected_ring_t> & projected, std::array<ComPtr<ID2D1PathGeometry>, GlowLayerCount> & geometries);
        void CreateRailGeometry(ID2D1Factory * factory, const std::vector<projected_ring_t> & projected, std::array<ComPtr<ID2D1PathGeometry>, GlowLayerCount> & geometries);

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    private:
        std::vector<ring_t> _Rings;
        std::vector<projected_ring_t> _ProjectedRings;

        std::atomic<size_t> _NextRing { 0 };

        double _TunnelRotation       = 0.;

        // One shared normalized physical-depth offset advances the complete ring lattice while every ring retains its fixed base phase.
        double _RingPhaseOffset      = 0.;

        // Physical spring system.
        double _CenterKickX          = 0.;
        double _CenterKickY          = 0.;
        double _CenterKickVelocityX  = 0.;
        double _CenterKickVelocityY  = 0.;

        double _PreviousCenterKickTrigger = 0.;

        uint32_t _CenterKickSeed = 0x65A4B31Du;

        /* Shared low-amplitude angular sway around the moving centerline. */
        double _CenterRotationSway = 0.;

        double _RotationDirection       = 1.;
        double _RotationDirectionTarget = 1.;
        double _RotationSwitchCooldown  = 0.;
        double _PreviousRotationTrigger = 0.;

        double _BassResponse = 1.65; // Bass sensitivity control: < 1.0 = less reactive bass, 1.0 = normal response, > 1.0 = more reactive bass.
        double _BeatResponse = 1.45; // Bass detection sensitivity

    private:
        ComPtr<ID2D1SolidColorBrush> _RingBrush;
        ComPtr<ID2D1SolidColorBrush> _RailBrush;

        ComPtr<ID2D1GradientStopCollection> _CoreStops;
        ComPtr<ID2D1RadialGradientBrush> _CoreBrush;

        ComPtr<ID2D1GradientStopCollection> _FlashStops;
        ComPtr<ID2D1RadialGradientBrush> _FlashBrush;

        ComPtr<ID2D1StrokeStyle> _RoundStrokeStyle;
    };
}
