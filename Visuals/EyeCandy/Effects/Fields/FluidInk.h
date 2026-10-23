
/** $VER: FluidInk.h (2026.10.22) **/

#pragma once

#include "Effect.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace FluidInk
{
    using namespace EyeCandy;

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

        static constexpr size_t BuiltInCount = 15;

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

    class effect_t final : public EyeCandy::effect_t
    {
    public:
        effect_t();
        ~effect_t() override final;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t && other) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t && other) = delete;

        void Reset() noexcept override final;
        void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk_impl & chunk) noexcept override final;

        std::wstring GetStatus() noexcept override final;

    private:
        struct work_t
        {
            double Time;
            double InverseWidth;
            double InverseHeight;
            double CoordinateScale;
            double HorizontalScale;
            double FlowSpeed;
            double BassPush;
            double MidCurl;
            double TrebleDetail;
            double TransientForce;
            double ForcePositionX;
            double ForcePositionY;
            double ColorPhase;
            double BalanceOffset;
            bool HasImpulse;
        };

        void Update(double time);

        void StartWorkers();
        void StopWorkers() noexcept;
        void Worker() noexcept;

        void GenerateRows(const work_t & work, uint32_t firstRow, uint32_t lastRow) noexcept;

        uint32_t Colorize(double density, double stream, double highlight) const noexcept;

    private:
        analyzer_t _Analyzer;

        static constexpr size_t BytesPerPixel = sizeof(uint32_t);

        UINT32 _RasterWidth;
        UINT32 _RasterHeight;

        std::vector<uint32_t> _Pixels;

        audio_t _Audio {};

    private:
        static constexpr double TransitionInterval = 3.; // seconds

        double _TransitionTime = 1.;

        size_t _ThemeIndex = 0;
        theme_t _Theme { theme_t::BuiltIn(_ThemeIndex) };

        double _NextTransitionTime = TransitionInterval;
        size_t _NextThemeIndex = 0;

        theme_t _FromTheme = { _Theme };
        theme_t _ToTheme{ _Theme };

    private:
        work_t _Work {};
        std::atomic<uint32_t> _NextRow { 0 };

    private:
        ComPtr<ID2D1Bitmap> _BackBuffer;
    };
}
