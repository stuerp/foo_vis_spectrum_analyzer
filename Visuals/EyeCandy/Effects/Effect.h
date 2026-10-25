
/** $VER: Effect.h (2026.10.23) P. Stuer - Base class for the Eye Candy effects. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 4746 5039 5045 5246 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>

#include <Windows.h>
#include <d2d1.h>

#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <array>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

#include <SDK/audio_chunk.h>

#include "Effects.h"

namespace EyeCandy
{
    constexpr double Pi = std::numbers::pi_v<double>;
    constexpr double TwoPi = 2. * Pi;
    constexpr double Epsilon = std::numeric_limits<double>::epsilon();

    struct interaction_t
    {
        double X       = 0.;
        double Y       = 0.;
        double Zoom    = 1.;
        double Impulse = 0.;
    };

    enum class Quality
    {
        Low = 0,
        Medium = 1,
        High = 2,

        Min = Low,
        Max = High
    };

    enum class Speed
    {
        Slow = 0,
        Normal = 1,
        Fast = 2,

        Min = Slow,
        Max = Fast
    };

    enum class Brightness
    {
        Low,
        Medium,
        High,

        Min = Low,
        Max = High
    };

    enum class Density
    {
        Low,
        Medium,
        High,

        Min = Low,
        Max = High
    };

    struct theme_t
    {
        std::wstring Name;
    };

    struct color_theme_t : theme_t
    {
        float BackgroundHueA = 0.f;
        float BackgroundHueB = 0.f;
        float RingHue        = 0.f;
        float RailHue        = 0.f;
        float CoreHue        = 0.f;
        float FlashHue       = 0.f;
        float HueRange       = 0.f;
        float Saturation     = 1.f;
        float Brightness     = 1.f;

        bool Monochrome = false;
    };

    enum class ColorTheme : uint8_t
    {
        Synthwave,
        Cyberpunk,
        Tron,
        Toxic,
        Inferno,
        Monochrome,
        Count
    };

    enum class ThemeRole : uint8_t
    {
        BackgroundA, BackgroundB, Ring, Rail, Core, Flash
    };

    struct hsv_t
    {
        float H = 0.f;
        float S = 0.f;
        float V = 0.f;
    };

    const D2D1_COLOR_F Black = D2D1::ColorF(D2D1::ColorF::Black);
    const D2D1_COLOR_F White = D2D1::ColorF(D2D1::ColorF::White);

    inline float Clamp(float value) noexcept
    {
        return std::clamp(value, 0.f, 1.f);
    }

    inline double Clamp(double value) noexcept
    {
        return std::clamp(value, 0., 1.);
    }

    inline double ClampAudio(double value) noexcept
    {
        if (!std::isfinite(value))
            return 0.;

        return std::clamp(value, -2., 2.);
    }

    inline double Length(double x, double y) noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    inline float Fraction(float value) noexcept
    {
        return value - std::floor(value);
    }

    inline double Fraction(double value) noexcept
    {
        return value - std::floor(value);
    }

    inline double Interpolate(double a, double b, double deltaTime, double rate) noexcept
    {
        return std::lerp(a, b, 1. - std::exp(-rate * deltaTime));
    }

    inline bool IsFinite(D2D1_POINT_2F point) noexcept
    {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    inline constexpr float Lerp(float from, float to, float amount) noexcept
    {
        return from + (to - from) * amount;
    }

    inline constexpr double Lerp(double from, double to, double amount) noexcept
    {
        return from + (to - from) * amount;
    }

    static double ExponentialSmoothing(double from, double to, double deltaTime, double response) noexcept
    {
        const double Amount = 1. - std::exp(-std::max(0., deltaTime) * response);

        return Lerp(from, to, Amount);
    }

    static float InterpolateHue(float from, float to, float amount) noexcept
    {
        from = Fraction(from);
        to   = Fraction(to);

        float Difference = to - from;

        if (Difference > 0.5f)
            Difference -= 1.0f;
        else
        if (Difference < -0.5f)
            Difference += 1.0f;

        return Fraction(from + Difference * amount + 1.0f);
    }

    static hsv_t InterpolateHSV(const hsv_t & from, const hsv_t & to, float amount) noexcept
    {
        amount = Clamp(amount);

        return
        {
            InterpolateHue(from.H, to.H, amount),

            Lerp(from.S, to.S, amount),
            Lerp(from.V, to.V, amount)
        };
    }

    static uint8_t ToByte(float value) noexcept
    {
        return (uint8_t) ((255.f * value) + 0.5f);
    }

    static uint8_t ToByte(double value) noexcept
    {
        return (uint8_t) ((255. * value) + 0.5);
    }

    static uint8_t ToRGB(double value) noexcept
    {
        return (uint8_t) (255. * std::pow(Clamp(value), 1. / 2.2));
    }

    static uint8_t GetChannel(uint32_t color, unsigned int shift) noexcept
    {
        return (uint8_t) ((color >> shift) & 0xFFu);
    }

    static D2D1_COLOR_F SetAlpha(D2D1_COLOR_F color, float alpha) noexcept
    {
        color.a = Clamp(alpha);

        return color;
    }

    /// <summary>
    /// 
    /// </summary>
    static uint32_t PackColor(double r, double g, double b) noexcept
    {
        const uint32_t R = ToByte(r);
        const uint32_t G = ToByte(g);
        const uint32_t B = ToByte(b);

        return 0xFF000000u | (R << 16u) | (G << 8u) | B;
    }

    /// <summary>
    /// 
    /// </summary>
    static uint32_t PackRGB(double r, double g, double b) noexcept
    {
        const uint32_t R = ToRGB(r);
        const uint32_t G = ToRGB(g);
        const uint32_t B = ToRGB(b);

        return 0xFF000000u | (R << 16u) | (G << 8u) | B;
    }

    /// <summary>
    /// Converts an HSV color to a normalized Direct2D color.
    /// </summary>
    static D2D1_COLOR_F HSVToColor(float h, float s, float v, float a = 1.f) noexcept
    {
        h = Fraction(h); // Wrap hue into [0, 1].
        s = Clamp(s);
        v = Clamp(v);

        const float ScaledHue = h * 6.0f;
        const auto  Sector    = (int) ScaledHue;
        const float Fraction  = ScaledHue - (float) Sector;

        const float ChromaReduction = v * s;

        const float p = v - ChromaReduction;
        const float q = v - ChromaReduction * Fraction;
        const float t = v - ChromaReduction * (1.f - Fraction);

        switch (Sector)
        {
            case 0:  return { v, t, p, Clamp(a) };
            case 1:  return { q, v, p, Clamp(a) };
            case 2:  return { p, v, t, Clamp(a) };
            case 3:  return { p, q, v, Clamp(a) };
            case 4:  return { t, p, v, Clamp(a) };
            default: return { v, p, q, Clamp(a) }; // 5
        }
    }

    /// <summary>
    /// Converts a color from HSV to RGB.
    /// </summary>
    static void HSVToRGB(double h, double s, double v, double & r, double & g, double & b) noexcept
    {
        h = Fraction(h); // Wrap hue into [0, 1].
        s = Clamp(s);
        v = Clamp(v);

        const double ScaledHue = h * 6.0;
        const auto   Sector    = (int) ScaledHue;
        const double Fraction  = ScaledHue - Sector;

        const double ChromaReduction = v * s;

        const double p = v - ChromaReduction;
        const double q = v - ChromaReduction * Fraction;
        const double t = v - ChromaReduction * (1. - Fraction);

        switch (Sector)
        {
            case 0:  { r = v; g = t; b = p; break; }
            case 1:  { r = q; g = v; b = p; break; }
            case 2:  { r = p; g = v; b = t; break; }
            case 3:  { r = p; g = q; b = v; break; }
            case 4:  { r = t; g = p; b = v; break; }
            default: { r = v; g = p; b = q; break; } // 5
        }
    }

    /// <summary>
    /// Base class for all effects.
    /// </summary>
    class effect_t
    {
    public:
        effect_t() = default;

        effect_t(const effect_t &) = delete;
        effect_t(effect_t &&) = delete;

        effect_t & operator=(const effect_t &) = delete;
        effect_t & operator=(effect_t &&) = delete;

        virtual ~effect_t() = default;

        void Resize(float width, float height) noexcept
        {
            _Width  = width;
            _Height = height;

            _CenterX = width  * 0.5f;
            _CenterY = height * 0.5f;

            _Rectangle = D2D1::RectF(0.f, 0.f, _Width, _Height);

            Reset();
        }

        virtual void Render(ID2D1RenderTarget * renderTarget, double deltaTime, audio_chunk & chunk) noexcept = 0;

        virtual void Reset() noexcept { };

        double GetTime() const noexcept
        {
            return _AnimationTime;
        }

        virtual std::wstring GetStatus() noexcept
        {
            return { };
        }

        void SetQuality(Quality value) noexcept
        {
            if (_Quality == value)
                return;

            _Quality = value;

            Reset();
        }

        void SetSpeed(Speed value) noexcept
        {
            if (_Speed == value)
                return;

            _Speed = value;

            Reset();
        }

        void SetBrightness(Brightness value) noexcept
        {
            if (_Brightness == value)
                return;

            _Brightness = value;

            Reset();
        }
        void SetDensity(Density value)
        {
            if (_Density == value)
                return;

            _Density = value;

            Reset();
        }

        interaction_t _Interaction {};

        // Factory method
        static std::unique_ptr<effect_t> Create(Effect effect) noexcept;

    public:

    protected:
        HRESULT CreateDeviceDependentResources(ID2D1RenderTarget * renderTarget);
        void DeleteDeviceDependentResources() noexcept;

        virtual void RenderBackground(ID2D1RenderTarget * renderTarget) noexcept;
        virtual void RenderScanlines(ID2D1RenderTarget * renderTarget) noexcept;
        virtual void RenderFrame(ID2D1RenderTarget * renderTarget, const D2D1_COLOR_F & color) noexcept;

        void UpdateThemeTransition(double deltaTime) noexcept;
        void BeginThemeTransition(ColorTheme theme) noexcept;

        void SetColorTheme(ColorTheme theme) noexcept;
        void NextColorTheme() noexcept;
        void PreviousColorTheme() noexcept;

        ColorTheme GetColorTheme() const noexcept
        {
            return _ColorTheme;
        }

        D2D1_COLOR_F GetThemeColor(ThemeRole role, float hueOffset, float saturation, float value, float alpha) const noexcept;
        D2D1_COLOR_F GetThemeColor(const color_theme_t & theme, float baseHue, float hueOffset, float saturation, float value, float alpha) const noexcept;

        static D2D1_COLOR_F LerpColor(const D2D1_COLOR_F & from, const D2D1_COLOR_F & to, float amount) noexcept;
        static const color_theme_t & GetTheme(ColorTheme theme) noexcept;
        static const std::wstring GetThemeName(ColorTheme theme) noexcept;

        static inline float SmoothStep(float lower, float upper, float value) noexcept;

        static inline uint32_t Hash(uint32_t value) noexcept;
        static inline double Random(uint32_t & state) noexcept;
        static inline double RandomSigned(uint32_t & state) noexcept;

    protected:
        float _Width  = 0.f;
        float _Height = 0.f;

        float _CenterX = 0.f;
        float _CenterY = 0.f;

        D2D1_RECT_F _Rectangle {};

        double _AnimationTime = 0.;

        double _Peak                = 0.; // [0, 1], Overall peak amplitude. (The loudest individual sample in a chunk.
        double _RMS                 = 0.; // [0, 1], RMS loudness
        double _StereoWidth         = 0.; // [0, 1]
        double _Correlation         = 1.; // [0, 1]

        double _BassEnergy          = 0.; // [0, 1] Combined bass energy
        double _MidEnergy           = 0.; // [0, 1] Combined midrange energy
        double _TrebleEnergy        = 0.; // [0, 1] Combined treble energy
        double _PhaseEnergy         = 0.; // [0, 1]

        double _SpectralFlux        = 0.; // [0, 1] How much the frequency spectrum increased since the previous call.

        double _SmoothedPeak        = 0.; // [0, 1]
        double _SmoothedRMS         = 0.; // [0, 1]

        double _SmoothedBass        = 0.; // [0, 1]
        double _SmoothedMid         = 0.; // [0, 1]
        double _SmoothedTreble      = 0.; // [0, 1]

        double _SmoothedWidth       = 0.; // [0, 1]
        double _SmoothedCorrelation = 1.; // [0, 1]

        double _SmoothedSpectralFlux = 0.; // [0, 1]

        static constexpr size_t FrequencyBandCount = 8;

        std::array<double, FrequencyBandCount> _Bands{};            // Eight frequency-band levels: 0-2 = bass, 3-5 = midrange, 6/7 = treble
        std::array<double, FrequencyBandCount> _SmoothedBands{};

        // Musical events:
        // _BeatAverage estimates the normal or local bass level of the current music.
        // A fixed bass threshold would not work reliably because different recordings have very different levels:
        // - one track may have quiet bass,
        // - another may have heavily compressed bass,
        // - another may have a continuous low-frequency drone,
        // - another may contain isolated kick drums.
        // Instead, _BeatAverage follows the recent bass level and provides an adaptive reference.
        double _BeatAverage          = 0.;
        double _BeatEnvelope         = 0.;
        double _BeatPulse            = 0.; // Represents a qualified beat event.
        double _BeatCooldown         = 0.;
        double _PreviousBassEnvelope = 0.;

        // Derived animation controls
        double _BassImpulse          = 0.; // Represents transient visual energy.
        double _BassExpansion        = 0.;
        double _BassTwist            = 0.;

        bool _HasAudio = false;

        double _TravelDistance       = 0.;

        double _CameraShake          = 0.;
        double _CameraRoll           = 0.;

    protected:
        Quality _Quality       = Quality::Medium;
        Speed _Speed           = Speed::Normal;
        Brightness _Brightness = Brightness::Medium;
        Density _Density       = Density::Medium;

        static constexpr std::array<const wchar_t *, 3> Qualities =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

        static constexpr std::array<const wchar_t *, 3> Speeds =
        {{
            L"Slow",
            L"Normal",
            L"Fast"
        }};

        static constexpr std::array<const wchar_t *, 3> Brightnesses =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

        static constexpr std::array<const wchar_t *, 3> DensityNames =
        {{
            L"Low",
            L"Medium",
            L"High"
        }};

    protected:
        ColorTheme _ColorTheme = ColorTheme::Synthwave;
        ColorTheme _ThemeFrom  = ColorTheme::Synthwave;
        ColorTheme _ThemeTo    = ColorTheme::Synthwave;

        static constexpr double ThemeInterval = 3.0;
        static constexpr double ThemeTransitionDuration = 1.2;
        static constexpr double ThemeResourceUpdateInterval = 1.0 / 30.0;

        double _ThemeElapsed           = 0.;
        double _ThemeTransitionElapsed = ThemeTransitionDuration;
        double _ThemeResourceElapsed   = 0.;
        double _ThemeBlend             = 1.;

        bool _ThemeTransitionActive = false;
        bool _ThemeResourcesDirty   = true;

    protected:
        std::vector<std::thread> _Workers;
        std::mutex _WorkerMutex;
        std::condition_variable _WorkerCondition;
        std::condition_variable _CompletionCondition;
        size_t _PendingWorkers = 0;
        size_t _Generation = 0;
        bool _IsStopping = false;

    protected:
        ID2D1RenderTarget * _RenderTarget = nullptr;

        ComPtr<ID2D1Factory> _D2DFactory;
        ComPtr<ID2D1LinearGradientBrush> _BackgroundBrush;
        ComPtr<ID2D1SolidColorBrush> _FrameBrush;

    private:
        ComPtr<ID2D1SolidColorBrush> _ScanlineBrush;
        ComPtr<ID2D1GradientStopCollection> _BackgroundStops;
    };

    /// <summary>
    /// Converts v into a smoothly interpolated value between 0 and 1. Unlike a linear interpolation, it starts and ends gradually, avoiding sharp transitions.
    /// </summary>
    float effect_t::SmoothStep(float lower, float upper, float value) noexcept
    {
        if (lower == upper)
            return value >= upper ? 1.f : 0.f;

        const float Amount = Clamp((value - lower) / (upper - lower));

        return Amount * Amount * (3.f - (2.f * Amount));
    }

    /// <summary>
    /// 
    /// </summary>
    uint32_t effect_t::Hash(uint32_t v) noexcept
    {
        v ^= v >> 16;
        v *= 0x7FEB352Du;
        v ^= v >> 15;
        v *= 0x846CA68Bu;

        return v ^ (v >> 16);
    }

    /// <summary>
    /// Returns a value in the [0, 1] range using a small, fast deterministic pseudo-random number generator.
    /// </summary>
    double effect_t::Random(uint32_t & seed) noexcept
    {
        seed = seed * 1664525u + 1013904223u;

        return (double)(seed >> 8) * (1. / 16777216.);
    }

    /// <summary>
    /// Returns a value in the [-1, 1] range using a small, fast deterministic pseudo-random number generator.
    /// </summary>
    double effect_t::RandomSigned(uint32_t & seed) noexcept
    {
        return (Random(seed) * 2.) - 1.;
    }
}
