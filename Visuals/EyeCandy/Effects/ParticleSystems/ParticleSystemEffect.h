
/** $VER: ParticleSystemEffect.h (2026.11.06) - Base class for a particle system effect */

#pragma once

#include "Effect.h"

#include <wrl/client.h>

namespace ParticleSystem
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    class effect_t : public EyeCandy::effect_t
    {
    public:
        effect_t() = default;
        ~effect_t() =default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

    protected:
        D2D1_POINT_2F _Center {};

        double _CameraRollVelocity   = 0.;
        double _CameraRollDirection  = 1.;

        double _FlightYaw            = 0.;
        double _FlightYawVelocity    = 0.;
        double _FlightPitch          = 0.;
        double _FlightPitchVelocity  = 0.;

        bool _FlightMotionEnabled    = true;

        enum class FlightMotionIntensity
        {
            Subtle,
            Normal,
            Strong,
        };

        FlightMotionIntensity _FlightMotionIntensity = FlightMotionIntensity::Normal;

        static constexpr double HorizontalVanishingPointRange = .25; // 25% of the viewport width.
        static constexpr double HVerticalVanishingPointRange  = .25; // 25% of the viewport height.

    protected:
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

        static constexpr size_t ThemeCount = ThemeHues.size();
        static constexpr double ThemeTransitionTime = 1.0;

    protected:
        ComPtr<ID2D1GradientStopCollection> _BackgroundStops;
        ComPtr<ID2D1RadialGradientBrush> _BackgroundBrush;
    };
}
