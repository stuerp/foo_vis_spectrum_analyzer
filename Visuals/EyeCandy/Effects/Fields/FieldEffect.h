
/** $VER: FieldEffect.h (2026.10.23) - Base class for a field effect */

#pragma once

#include "Effect.h"

#include <d2d1.h>
#include <wrl/client.h>

namespace Field
{
    using namespace EyeCandy;

    using Microsoft::WRL::ComPtr;

    struct audio_t
    {
        double Loudness = 0.;
        double Bass     = 0.;
        double Mid      = 0.;
        double Treble   = 0.;
        double Balance  = 0.;
        double Peak     = 0.;
    };

    class analyzer_t
    {
    public:
        audio_t Analyze(const audio_chunk_impl & chunk, double deltaTime) noexcept;
        void Decay(double deltaTime) noexcept;

    private:
        audio_t _Audio {};

        double _LowState  = 0.;
        double _HighState = 0.;
    };

    struct theme_t : EyeCandy::theme_t
    {
        double BaseHue    =  0.;        // Absolute position on the color wheel, [0, 1]

        double HueSpan    =   .34;      // Distance across the color wheel
        double Saturation =   .82;
        double Brightness =  1.;

        double BassHue    = -.08;
        double MidHue     =   .10;
        double TrebleHue  =   .05;

        double BassRed    =   .14;
        double MidGreen   =   .08;
        double TrebleBlue =   .12;

        static constexpr size_t BuiltInCount = 30;

        static inline theme_t BuiltIn(size_t index) noexcept
        {
            switch (index % BuiltInCount)
            {
                // Broad ultraviolet-to-cyan palette with strong audio movement.
                default:
                case 0:
                    return
                    {
                        L"Neon",
                         .720, .340, .84, 1.00,
                        -.080, .100, .050,
                         .140, .080, .120
                    };

                // Concentrated reds, oranges, and golds with bass-driven heat.
                case 1:
                    return
                    {
                        L"Fire",
                         .015, .120, .90, 1.05,
                        -.020, .040, .035,
                         .180, .050, .030
                    };

                // Deep cyan and blue with cool treble highlights.
                case 2:
                    return
                    {
                        L"Ocean",
                         .480, .180, .78, .98,
                        -.070, .030, .050,
                         .040, .090, .160
                    };

                // Saturated violet and magenta with bright electric-blue edges.
                case 3:
                    return
                    {
                        L"Ultraviolet",
                         .755, .205, .92, 1.04,
                        -.055, .075, .105,
                         .095, .025, .210
                    };

                // Dark blue-green shadows opening into luminous emerald.
                case 4:
                    return
                    {
                        L"Aurora",
                         .355, .245, .80, 1.00,
                        -.090, .135, .070,
                         .020, .170, .105
                    };

                // Hot magenta, cyan, and violet inspired by retro digital color.
                case 5:
                    return
                    {
                        L"Synthwave",
                         .805, .275, .91, 1.06,
                         .065, -.085, .120,
                         .170, .025, .190
                    };

                // Dim crimson and violet with sharp blue transient flashes.
                case 6:
                    return
                    {
                        L"Blood Moon",
                         .965, .105, .86, .82,
                        -.025, .055, .155,
                         .210, .015, .095
                    };

                // Wide spectral movement with balanced RGB audio accents.
                case 7:
                    return
                    {
                        L"Prismatic",
                         .000, .920, .88, 1.00,
                         .115, -.180, .240,
                         .105, .105, .105
                    };

                // Muted cyan, lavender, and pink with a soft luminous response.
                case 8:
                    return
                    {
                        L"Dreamscape",
                         .515, .335, .56, 1.08,
                        -.035, .080, .095,
                         .070, .100, .145
                    };

                // Dense greens and turquoise with strong midrange illumination.
                case 9:
                    return
                    {
                        L"Bioluminescence",
                         .305, .205, .87, .91,
                        -.100, .145, .080,
                         .015, .215, .155
                    };

                // Rich indigo shadows with violet and blue stellar highlights.
                case 10:
                    return
                    {
                        L"Deep Space",
                         .625, .185, .83, .76,
                        -.030, .090, .165,
                         .055, .025, .205
                    };

                // Warm amber and bronze with restrained saturation.
                case 11:
                    return
                    {
                        L"Molten Gold",
                         .075, .085, .76, 1.08,
                        -.015, .025, -.020,
                         .230, .105, -.025
                    };

                // Acidic green and yellow with aggressive reactive contrast.
                case 12:
                    return
                    {
                        L"Toxic",
                         .205, .155, .96, 1.02,
                        -.090, .120, -.065,
                         .055, .220, .010
                    };

                // Pale cyan, silver-blue, and crisp transient highlights.
                case 13:
                    return
                    {
                        L"Glacier",
                         .495, .115, .48, 1.12,
                        -.035, .025, .090,
                         .025, .095, .180
                    };

                // Teal shadows and coral highlights with opposing audio shifts.
                case 14:
                    return
                    {
                        L"Solar Reef",
                         .455, .585, .82, 1.03,
                         .135, -.115, .180,
                         .165, .115, .145
                    };
                // Deep navy through electric cyan, with crisp blue transients.
                case 15:
                    return
                    {
                        L"Electric Abyss",
                         .555, .175, .91, .92,
                        -.045, .060, .125,
                         .025, .075, .220
                    };

                // Warm coral and peach with glowing golden midrange accents.
                case 16:
                    return
                    {
                        L"Coral Sunset",
                         .985, .145, .78, 1.06,
                         .025, .060, -.035,
                         .180, .110, .045
                    };

                // Dark violet opening into saturated orchid and pink.
                case 17:
                    return
                    {
                        L"Orchid",
                         .795, .145, .86, .97,
                        -.035, .085, .125,
                         .145, .040, .165
                    };

                // Warm copper and oxidized turquoise with contrasting movement.
                case 18:
                    return
                    {
                        L"Verdigris",
                         .080, .360, .71, .94,
                         .035, .180, -.095,
                         .185, .125, .085
                    };

                // Soft rose, lavender, and pale blue with restrained saturation.
                case 19:
                    return
                    {
                        L"Pastel Bloom",
                         .925, .420, .43, 1.13,
                         .045, .075, .105,
                         .095, .105, .130
                    };

                // Dense orange-red plasma with searing yellow highlights.
                case 20:
                    return
                    {
                        L"Solar Flare",
                         .025, .105, .97, 1.12,
                        -.030, .040, .075,
                         .245, .120, .025
                    };

                // Cold blue and violet with energetic white-blue edges.
                case 21:
                    return
                    {
                        L"Arctic Pulse",
                         .545, .155, .68, 1.10,
                        -.055, .035, .110,
                         .035, .105, .205
                    };

                // Emerald, jade, and muted gold inspired by mineral colors.
                case 22:
                    return
                    {
                        L"Jade",
                         .315, .125, .76, .94,
                        -.035, .075, -.060,
                         .050, .185, .065
                    };

                // Saturated amber through magenta with strong bass warmth.
                case 23:
                    return
                    {
                        L"Phoenix",
                         .035, .835, .92, 1.07,
                        -.075, .115, .185,
                         .225, .075, .105
                    };

                // Near-monochromatic blue with subtle cyan tonal movement.
                case 24:
                    return
                    {
                        L"Midnight Blue",
                         .595, .075, .84, .78,
                        -.020, .025, .055,
                         .030, .045, .190
                    };

                // Luminous purple, pink, and blue with broad spectral motion.
                case 25:
                    return
                    {
                        L"Cosmic Candy",
                         .790, .405, .88, 1.08,
                         .110, -.135, .175,
                         .155, .060, .185
                    };

                // Yellow-green through emerald with sharp acidic highlights.
                case 26:
                    return
                    {
                        L"Radioactive",
                         .175, .175, .98, 1.09,
                        -.055, .095, -.085,
                         .115, .235, .020
                    };

                // Restrained steel blue with pale silver illumination.
                case 27:
                    return
                    {
                        L"Moonlight",
                         .565, .090, .35, 1.05,
                        -.025, .035, .065,
                         .055, .085, .145
                    };

                // Burnt orange, dark red, and smoky purple.
                case 28:
                    return
                    {
                        L"Ember",
                         .005, .155, .84, .87,
                        -.035, .080, .115,
                         .230, .055, .055
                    };

                // Very wide hue travel for constantly changing spectral color.
                case 29:
                    return
                    {
                        L"Chromatic Storm",
                         .125, .980, .94, 1.02,
                         .175, -.245, .315,
                         .145, .125, .155
                    };
            }
        }

        /// <summary>
        /// Blends two themes over time.
        /// </summary>
        static inline theme_t Blend(const theme_t & a, const theme_t & b, double time) noexcept
        {
            time = std::clamp(time, 0., 1.);

            time = time * time * (3. - (2. * time));

            auto LerpHue = [time](double x, double y) noexcept -> double
            {
                double d = std::fmod(y - x + 1.5, 1.) - .5;
                double h = x + (d * time);

                return h - std::floor(h);
            };

            auto Lerp = [time](double x, double y) noexcept -> double
            {
                return std::lerp(x, y, time);
            };

            return
            {
                (time < 0.5f) ? a.Name : b.Name,

                LerpHue(a.BaseHue, b.BaseHue),

                Lerp(a.HueSpan,    b.HueSpan),
                Lerp(a.Saturation, b.Saturation),
                Lerp(a.Brightness, b.Brightness),
                Lerp(a.BassHue,    b.BassHue),
                Lerp(a.MidHue,     b.MidHue),
                Lerp(a.TrebleHue,  b.TrebleHue),
                Lerp(a.BassRed,    b.BassRed),
                Lerp(a.MidGreen,   b.MidGreen),
                Lerp(a.TrebleBlue, b.TrebleBlue)
            };
        }
    };

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
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget) noexcept;
        void DeleteDeviceDependentResources() noexcept;

    protected:
        analyzer_t _Analyzer;

        audio_t _Audio {};

    protected:
        static constexpr size_t BytesPerPixel = sizeof(uint32_t);

        UINT32 _RasterWidth  = 0;
        UINT32 _RasterHeight = 0;

        std::vector<uint32_t> _Pixels;

    protected:
        static constexpr double TransitionInterval = 3.; // seconds

        double _ThemeTime = 0.;

        double _TransitionTime = 1.;

        size_t _ThemeIndex = 0;
        theme_t _Theme { Field::theme_t::BuiltIn(_ThemeIndex) };

        double _NextTransitionTime = TransitionInterval;
        size_t _NextThemeIndex = 0;

        theme_t _FromTheme = { _Theme };
        theme_t _ToTheme{ _Theme };

    protected:
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
