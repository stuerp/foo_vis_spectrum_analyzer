
/** $VER: ColorSorting.cpp (2026.10.11) Color sort support **/

#include "pch.h"

#include <d2d1.h>

#include <algorithm>
#include <cmath>
#include <compare>
#include <limits>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <tuple>
#include <vector>

// std::vector<D2D1_COLOR_F> Colors = { ... };
//
// ColorSorting::Sort(Colors, { .mode = ColorSorting::Mode::HueThenLightness, .Direction = ColorSorting::Direction::Ascending, .AchromaticPlacement = ColorSorting::AchromaticPlacement::First });
// ColorSorting::Sort(Colors, { .mode = ColorSorting::Mode::ContrastAgainstBackground, .Direction = ColorSorting::Direction::Descending, .BackgroundColor = D2D1_COLOR_F{1.0f, 1.0f, 1.0f, 1.0f} });

namespace ColorSorting
{
    struct ok_lab_t
    {
        float L {};
        float a {};
        float b {};
    };

    struct ok_lch_t
    {
        float L {}; // Lightness
        float C {}; // Chroma
        float H {}; // Hue, In degrees, [0, 360], Zero for achromatic colors.
    };

    enum class Mode
    {
        RelativeLuminance,
        PerceivedLightness,
        FastBrightness,
        HueThenLightness,
        LightnessThenHue,
        HueThenChromaThenLightness,
        DistanceFromReference,
        ContrastAgainstBackground,
        RGBA
    };

    enum class Direction
    {
        Ascending,
        Descending
    };

    enum class AchromaticPlacement
    {
        First,
        Last
    };

    struct sort_options_t
    {
        Mode mode{ Mode::PerceivedLightness };

        Direction direction{ Direction::Ascending };

        AchromaticPlacement AchromaticPlacement{ AchromaticPlacement::First };

        // Used by distance_from_reference.
        D2D1_COLOR_F ReferenceColor{ 0.0f, 0.0f, 0.0f, 1.0f };

        // Used by contrast_against_background. It can also be used to composite translucent colors before contrast is calculated.
        D2D1_COLOR_F BackgroundColor{ 1.0f, 1.0f, 1.0f, 1.0f };

        // Hue is undefined near the neutral axis. Colors below this OKLCH chroma are grouped as achromatic.
        float ChromaticChromaThreshold{ 0.02f };

        // When true, translucent colors are composited over background before brightness, lightness, hue, chroma, distance, or contrast is measured.
        bool CompositeTranslucentColors{ false };

        // Stable sorting preserves input order when keys compare equal.
        bool UseStableSort{ true };
    };

    namespace Internal
    {
        constexpr float Clamp(const float value) noexcept
        {
            return std::clamp(value, 0.f, 1.f);
        }

        inline float FiniteOrZero(const float value) noexcept
        {
            return std::isfinite(value) ? value : 0.f;
        }

        inline float sRGBToLinear(float value) noexcept
        {
            value = Clamp(FiniteOrZero(value));

            return (value <= 0.04045f) ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
        }

        inline D2D1_COLOR_F Sanitize(D2D1_COLOR_F color) noexcept
        {
            color.r = Clamp(FiniteOrZero(color.r));
            color.g = Clamp(FiniteOrZero(color.g));
            color.b = Clamp(FiniteOrZero(color.b));
            color.a = Clamp(FiniteOrZero(color.a));

            return color;
        }

        /// <summary>
        /// Composites a foreground color over a background color.
        /// </summary>
        inline D2D1_COLOR_F CompositeOver(D2D1_COLOR_F foreground, D2D1_COLOR_F background) noexcept
        {
            foreground = Sanitize(foreground);
            background = Sanitize(background);

            // Straight-alpha source-over compositing, including background alpha.
            const float Alpha = foreground.a + background.a * (1.0f - foreground.a);

            if (Alpha <= std::numeric_limits<float>::epsilon())
                return { 0.0f, 0.0f, 0.0f, 0.0f };

            const auto Channel = [&](const float foregroundChannel, const float backgroundChannel) noexcept
            {
                return (foregroundChannel * foreground.a + backgroundChannel * background.a * (1.f - foreground.a)) / Alpha;
            };

            return
            {
                Channel(foreground.r, background.r),
                Channel(foreground.g, background.g),
                Channel(foreground.b, background.b),
                Alpha
            };
        }

        /// <summary>
        /// Evaluates the color against sort options.
        /// </summary>
        inline D2D1_COLOR_F EvaluateColor(const D2D1_COLOR_F color, const sort_options_t & options) noexcept
        {
            return options.CompositeTranslucentColors ? CompositeOver(color, options.BackgroundColor) : Sanitize(color);
        }

        /// <summary>
        /// Converts rRGB to OKLab.
        /// 
        /// Differs subtly from relative luminance:
        /// - Relative luminance represents physical light output weighted for human vision.
        /// - OKLab L is designed to behave more like perceived lightness.
        ///
        /// OKLab/OKLCH is generally more suitable for palette organization, even-looking ramps, and perceptual color comparisons than RGB or HSL.
        /// </summary>
        /// https://utilitiesbunker.com/kb/perceptual-color-lab-oklch
        inline ok_lab_t ToOKLab(const D2D1_COLOR_F & srgb) noexcept
        {
            const float r = sRGBToLinear(srgb.r);
            const float g = sRGBToLinear(srgb.g);
            const float b = sRGBToLinear(srgb.b);

            const float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
            const float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
            const float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

            const float l_root = std::cbrt(l);
            const float m_root = std::cbrt(m);
            const float s_root = std::cbrt(s);

            return
            {
                 0.2104542553f * l_root + 0.7936177850f * m_root - 0.0040720468f * s_root,
                 1.9779984951f * l_root - 2.4285922050f * m_root + 0.4505937099f * s_root,
                 0.0259040371f * l_root + 0.7827717662f * m_root - 0.8086757660f * s_root
            };
        }

        /// <summary>
        /// Converts sRGB to OKLCH. (Cylindrical form of OKLAB)
        /// </summary>
        /// https://utilitiesbunker.com/kb/perceptual-color-lab-oklch
        inline ok_lch_t ToOKLCH(const D2D1_COLOR_F & srgb) noexcept
        {
            const ok_lab_t Lab = ToOKLab(srgb);

            const float Chroma = std::hypot(Lab.a, Lab.b);

            float Hue = std::atan2(Lab.b, Lab.a) * 180.0f / std::numbers::pi_v<float>;

            if (Hue < 0.0f)
                Hue += 360.0f;

            return { Lab.L, Chroma, Chroma > 0.0f ? Hue : 0.0f };
        }

        /// <summary>
        /// Calculates the relative luminance.
        /// </summary>
        inline float RelativeLuminance(const D2D1_COLOR_F & srgb) noexcept
        {
            return 0.2126f * sRGBToLinear(srgb.r) + 0.7152f * sRGBToLinear(srgb.g) + 0.0722f * sRGBToLinear(srgb.b);
        }

        /// <summary>
        /// Fast approximate brightness. Operates directly on gamma-encoded RGB, so it is only an approximation. It is cheaper than linearizing every channel.
        /// </summary>
        inline float FastBrightness(const D2D1_COLOR_F & srgb) noexcept
        {
            const auto c = Sanitize(srgb);

            return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
        }

        /// <summary>
        /// Calculates the Euclidean distance between two colors.
        /// </summary>
        inline float OKLabDistance(const D2D1_COLOR_F & lhs, const D2D1_COLOR_F & rhs) noexcept
        {
            const ok_lab_t a = ToOKLab(lhs);
            const ok_lab_t b = ToOKLab(rhs);

            return std::hypot(a.L - b.L, a.a - b.a, a.b - b.b);
        }

        /// <summary>
        /// Calculates the contrast ratio of two colors.
        /// </summary>
        inline float ContrastRatio(const D2D1_COLOR_F & lhs, const D2D1_COLOR_F & rhs) noexcept
        {
            const float l1 = RelativeLuminance(lhs);
            const float l2 = RelativeLuminance(rhs);

            return (std::max(l1, l2) + 0.05f) / (std::min(l1, l2) + 0.05f);
        }

        struct Key
        {
            float first{};
            float second{};
            float third{};
            float fourth{};

            auto tuple() const noexcept
            {
                return std::tuple{ first, second, third, fourth };
            }
        };

        inline Key MakeKey(const D2D1_COLOR_F & srcColor, const sort_options_t & options)
        {
            const D2D1_COLOR_F Color = EvaluateColor(srcColor, options);

            switch (options.mode)
            {
                case Mode::RelativeLuminance:
                    return { RelativeLuminance(Color) };

                // For a visually smooth dark-to-light palette, convert sRGB to OKLab and sort by its L component.
                case Mode::PerceivedLightness:
                    return { ToOKLab(Color).L };

                // Fast approximate brightness. Operates directly on gamma-encoded RGB, so it is only an approximation. It is cheaper than linearizing every channel.
                case Mode::FastBrightness:
                    return { FastBrightness(Color) };

                // Groups reds together, then oranges, yellows, greens, and so on. Within each group, colors run dark to light.
                // Using OKLCH instead of HSL makes lightness and chroma more visually meaningful. HSL colors with the same numerical lightness can have very different perceived brightness.
                // Hue is unstable or meaningless for greys, black, and white. Optionally puts low-chroma colors into a separate group.
                case Mode::HueThenLightness:
                {
                    const ok_lch_t Value = ToOKLCH(Color);

                    const bool IsAchromatic = (Value.C < options.ChromaticChromaThreshold);

                    const float Group = IsAchromatic ?
                        (options.AchromaticPlacement == AchromaticPlacement::First ? 0.0f : 1.0f) :
                        (options.AchromaticPlacement == AchromaticPlacement::First ? 1.0f : 0.0f);

                    return { Group, IsAchromatic ? 0.0f : Value.H, Value.L, Value.C };
                }

                // Produces horizontal "brightness bands".
                case Mode::LightnessThenHue:
                {
                    const ok_lch_t Value = ToOKLCH(Color);

                    const bool IsAchromatic = (Value.C < options.ChromaticChromaThreshold);

                    const float NeutralHue = options.AchromaticPlacement == AchromaticPlacement::First ? -1.0f : 361.0f;

                    return { Value.L, IsAchromatic ? NeutralHue : Value.H, Value.C };
                }

                // Groups colors by family and orders muted colors before vivid colors.
                case Mode::HueThenChromaThenLightness:
                {
                    const ok_lch_t Value = ToOKLCH(Color);

                    const bool IsAchromatic = (Value.C < options.ChromaticChromaThreshold);

                    const float Group = IsAchromatic
                        ? (options.AchromaticPlacement == AchromaticPlacement::First ? 0.0f : 1.0f)
                        : (options.AchromaticPlacement == AchromaticPlacement::First ? 1.0f : 0.0f);

                    return { Group, IsAchromatic ? 0.0f : Value.H, Value.C, Value.L };
                }

                // Sort colors from most similar to least similar to a selected reference color.
                // Perceptual spaces are designed so numeric distance more closely corresponds to visual difference, although no model is perfectly uniform.
                // https://auricartisan.com/library/learn/articles/2026-05-24-cielab-and-lch-explained
                case Mode::DistanceFromReference:
                {
                    return { OKLabDistance(Color, EvaluateColor(options.ReferenceColor, options)) };
                }

                // For UI colors, sorting by contrast can be more useful than brightness.
                // For translucent colors, composite them over the background before calculating contrast.
                case Mode::ContrastAgainstBackground:
                {
                    const D2D1_COLOR_F Foreground = CompositeOver(srcColor, options.BackgroundColor);

                    return { ContrastRatio(Foreground, Sanitize(options.BackgroundColor)) };
                }

                // Sort by RGB lexicographical order.
                case Mode::RGBA:
                {
                    const auto Value = Sanitize(srcColor);

                    return { Value.r, Value.g, Value.b, Value.a };
                }
            }

            throw std::logic_error{ "Unhandled color_sort::Mode" };
        }
    }

    /// <summary>
    /// Converts rRGB to OKLab.
    /// </summary>
    inline ok_lab_t ToOKLab(const D2D1_COLOR_F & color) noexcept
    {
        return Internal::ToOKLab(Internal::Sanitize(color));
    }

    /// <summary>
    /// Converts rRGB to OKLCH.
    /// </summary>
    inline ok_lch_t ToOKLCH(const D2D1_COLOR_F & color) noexcept
    {
        return Internal::ToOKLCH(Internal::Sanitize(color));
    }

    /// <summary>
    /// Calculates the relative luminance.
    /// </summary>
    inline float RelativeLuminance(const D2D1_COLOR_F & color) noexcept
    {
        return Internal::RelativeLuminance(Internal::Sanitize(color));
    }

    /// <summary>
    /// Calculates the contrast ratio of two colors.
    /// </summary>
    inline float ContrastRatio(const D2D1_COLOR_F & foreground, const D2D1_COLOR_F & background) noexcept
    {
        return Internal::ContrastRatio(Internal::CompositeOver(foreground, background), Internal::Sanitize(background));
    }

    /// <summary>
    /// Calculates the Euclidean distance between two colors.
    /// </summary>
    inline float PerceptualDistance(const D2D1_COLOR_F & lhs, const D2D1_COLOR_F & rhs) noexcept
    {
        return Internal::OKLabDistance(Internal::Sanitize(lhs), Internal::Sanitize(rhs));
    }

    /// <summary>
    /// Sorts the specified colors.
    /// </summary>
    inline void Sort(std::vector<D2D1_COLOR_F> & colors, const sort_options_t & options = {})
    {
        struct decorated_color_t
        {
            D2D1_COLOR_F Color{};
            Internal::Key Key{};
        };

        std::vector<decorated_color_t> DecoratedColors;

        DecoratedColors.reserve(colors.size());

        std::ranges::transform(colors, std::back_inserter(DecoratedColors), [&](const D2D1_COLOR_F & color)
        {
            return decorated_color_t{ color, Internal::MakeKey(color, options) };
        });

        const auto Less = [&](const decorated_color_t & lhs, const decorated_color_t & rhs)
        {
            return options.direction == Direction::Ascending ? lhs.Key.tuple() < rhs.Key.tuple() : rhs.Key.tuple() < lhs.Key.tuple();
        };

        if (options.UseStableSort)
            std::ranges::stable_sort(DecoratedColors, Less);
        else
            std::ranges::sort(DecoratedColors, Less);

        std::ranges::transform(DecoratedColors, colors.begin(), [](const decorated_color_t & value)
        {
            return value.Color;
        });
    }
}
