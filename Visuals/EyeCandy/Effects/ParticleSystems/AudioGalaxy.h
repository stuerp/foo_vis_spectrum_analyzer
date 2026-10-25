
/** $VER: AudioGalaxy.h (2026.10.24) **/

#pragma once

#include "Effect.h"
#include "ThreeD.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include <d2d1.h>
#include <wrl/client.h>

namespace AudioGalaxy
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t final : public EyeCandy::effect_t, public EyeCandy::ThreeD
    {
    public:
        enum class FlightMotionIntensity
        {
            Subtle,
            Normal,
            Strong,
        };

        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept override final;
        void Reset() noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct star_t
        {
            double X = 0.;
            double Y = 0.;
            double Z = 0.;

            double PreviousZ   = 0.;
            double PreviousX   = 0.;
            double PreviousY   = 0.;

            double Age         = 0.;
            double Lifetime    = 0.;
            double Size        = 0.; // Size of the star

            double Angle       = 0.;
            double Radius      = 0.; // Distance from the center of the galaxy
            double Speed       = 0.;
            double Brightness  = 0.;
            double HueOffset   = 0.;

            double Height      = 0.;
            double Inclination = 0.;
            double NodeAngle   = 0.;
            size_t Band        = 0;

            uint32_t Seed = 0;
        };

    private:
        void AnalyzeAudio(const audio_chunk & chunk);
        void Update(double deltaTime);

        void GenerateWork(double deltaTime, double forwardSpeed) noexcept;
        void UpdateStar(size_t index, double deltaTime, double forwardSpeed) noexcept;

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void RespawnStar(star_t & star, size_t index, bool randomDepth) noexcept;

        float CurrentThemeHue() const noexcept;

        void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept override final;

        void RenderStars(ID2D1RenderTarget * renderTarget);
        void DrawStar(ID2D1RenderTarget * renderTarget, D2D1_POINT_2F position, const D2D1_COLOR_F & color, float radius, float feather) noexcept;
        void RenderFlash(ID2D1RenderTarget * renderTarget) noexcept;

        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;
        
        D2D1_COLOR_F GetStarColor(const star_t & star, double depthBrightness) const noexcept;
        
    private:
        std::vector<star_t> _Stars;

        std::atomic<size_t> _NextStar { 0 };

        static constexpr double MinBirthAge =  0.; // seconds
        static constexpr double MaxBirthAge = 10.; // seconds

        static constexpr double MinLifeTime =  7.; // seconds
        static constexpr double MaxLifeTime = 18.; // seconds

        static constexpr double MaxRadius       = 1.30;
        static constexpr double MaxAngularSpeed = 2.00;

        double _ScaleX               = 0.;
        double _ScaleY               = 0.;

        double _StarDeltaTime        = 0.;
        double _ForwardSpeed         = 0.;

        double _BassState            = 0.;
        double _DCState              = 0.;
        double _MidState             = 0.;

        double _PreviousPeak         = 0.;
        double _PreviousTreble       = 0.;

        double _TransientPulse       = 0.;
        double _TransientCooldown    = 0.;
        size_t _TransientStarIndex   = 0;

        double _CameraRollVelocity   = 0.;
        double _CameraRollDirection  = 1.;

        double _FlightYaw            = 0.;
        double _FlightYawVelocity    = 0.;
        double _FlightPitch          = 0.;
        double _FlightPitchVelocity  = 0.;

        static constexpr double HorizontalVanishingPointRange = .25; // 25% of the viewport width.
        static constexpr double HVerticalVanishingPointRange  = .25; // 25% of the viewport height.

        D2D1_POINT_2F _Center {};

    private:
        inline static constexpr std::array<float, 10> ThemeHues =
        { {
            0.54f, // Andromeda blue.
            0.66f, // Whirlpool violet.
            0.78f, // Orion magenta.
            0.91f, // Rosette crimson.
            0.04f, // Sombrero amber.
            0.12f, // Cartwheel gold.
            0.33f, // Aurora green.
            0.47f, // Triangulum cyan.
            0.70f, // Pinwheel indigo.
            0.85f, // Cigar rose.
        } };

    private:
        static constexpr double TransitionInterval = 5.; // seconds

        double _TransitionTime = 0.;

        static constexpr size_t ThemeCount = ThemeHues.size();
        static constexpr double ThemeTransitionTime = 1.0;

        size_t _ThemeIndex = 1;

    private:
        ComPtr<ID2D1GradientStopCollection> _BackgroundStops;
        ComPtr<ID2D1RadialGradientBrush> _BackgroundBrush;
    };
}
