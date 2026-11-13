
/** $VER: Effects.h (2026.11.07) P. Stuer **/

#pragma once

namespace EyeCandy
{
    enum class Category
    {
        DynamicSystems = 0,
        Fields = 1,
        ParticleSystems = 2,
        Retro = 3,
        Trails = 4,

        Min = DynamicSystems,
        Max = Trails,
    };

    enum class Effect
    {
        Waveform = 0,

        // Dynamic Systems
        AizawaAttractor,
        DoublePendulum,
        HalvorsenAttractor,
        LorenzAttractor,
        RosslerAttractor,
        ThomasAttractor,

        CliffordAttractor,
        DeJongAttractor,

        // Trails
        LissajousTrails,
        RibbonFlow,
        Spirograph,

        // Particle Systems
        Fireflies,
        Fireworks,
        ParticleVortex,
        Starfield,
        AudioGalaxy,
        Swarm,

        // Retro
        NeonTunnel,
        VectorGrid,
        Metaballs,

        // Fields
        Plasma,
        FluidInk,
        Aurora,

        Monitor,

        HiddenLineLandscape,
        Rotozoomer,
        InterferenceMoire,
        FlipDotMatrix,
        DitherField,
        FlowerScope,
        HarmonicHarp,

        DuffingOscillator,
        ChuaCircuit,
        VanDerPolOscillator,
        GrayScottReactionDiffusion,
        RotatingTunnel,

        Min = Waveform,
        Max = RotatingTunnel,
    };

    constexpr double Pi    = std::numbers::pi_v<double>;
    constexpr double TwoPi = 2. * Pi;
    constexpr double Phi   = 1.61803398875; // Golden ratio: φ = (1 + sqrt(5)) / 2

    constexpr double Epsilon = std::numeric_limits<double>::epsilon();

    constexpr double LoFrequency     =     20.;
    constexpr double HiFrequency     = 20'000.;

    constexpr double FrequencyRatio  = HiFrequency / LoFrequency;

    constexpr double BassFrequency   =    250.; // 16 to 250 Hz (C0 to C4), https://en.wikipedia.org/wiki/Bass_(sound)
    constexpr double TrebleFrequency =  6'000.; // 6 to 20 kHz, https://en.wikipedia.org/wiki/Treble_(sound)

    // Cutoff frequencies for the first-order low-pass filters.
    constexpr double SubBassCutoff  =   35.; // Hz, Removes DC and very-low-frequency movement.
    constexpr double BassCutoff     =  220.; // Hz, Separates bass from the mid frequecies.
    constexpr double MidCutoff      = 2400.; // Hz, Separates the mids from treble.

    constexpr size_t BytesPerPixel = sizeof(uint32_t);

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

    template <typename T> inline constexpr T Clamp(T value) noexcept
    {
        return std::clamp(value, (T) 0., (T) 1.);
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

    template <typename T> inline constexpr T Fraction(T value) noexcept
    {
        return value - std::floor(value);
    }

    inline constexpr double Interpolate(double a, double b, double deltaTime, double rate) noexcept
    {
        return std::lerp(a, b, 1. - std::exp(-rate * deltaTime));
    }

    inline bool IsFinite(D2D1_POINT_2F point) noexcept
    {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    /// <summary>
    /// Hashes the specified value.
    /// </summary>
    inline uint32_t Hash(uint32_t v) noexcept
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
    inline double Random(uint32_t & seed) noexcept
    {
        seed = seed * 1664525u + 1013904223u;

        return (double)(seed >> 8) * (1. / 16777216.);
    }

    /// <summary>
    /// Returns a value in the [-1, 1] range using a small, fast deterministic pseudo-random number generator.
    /// </summary>
    inline double RandomSigned(uint32_t & seed) noexcept
    {
        return (Random(seed) * 2.) - 1.;
    }

    template <typename T> inline constexpr T Lerp(T from, T to, T amount) noexcept
    {
        return from + (to - from) * amount;
    }

    inline D2D1_COLOR_F Lerp(const D2D1_COLOR_F & from, const D2D1_COLOR_F & to, float amount) noexcept
    {
        amount = Clamp(amount);

        return D2D1::ColorF(Lerp(from.r, to.r, amount), Lerp(from.g, to.g, amount), Lerp(from.b, to.b, amount), Lerp(from.a, to.a, amount));
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

    inline uint8_t GetChannel(uint32_t color, unsigned int shift) noexcept
    {
        return (uint8_t) ((color >> shift) & 0xFFu);
    }

    inline D2D1_COLOR_F SetAlpha(D2D1_COLOR_F color, float alpha) noexcept
    {
        color.a = Clamp(alpha);

        return color;
    }

    inline uint32_t ToByte(double value) noexcept
    {
        return (uint32_t) std::clamp(value * 255., 0., 255.);
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

    inline uint32_t ToRGB(double value) noexcept
    {
        return (uint32_t) (std::pow(Clamp(value), 1. / 2.2) * 255.);
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
    /// Converts value into a smoothly interpolated value between 0 and 1. Unlike a linear interpolation, it starts and ends gradually, avoiding sharp transitions.
    /// </summary>
    template <typename T>
    T SmoothStep(T lower, T upper, T value) noexcept
    {
        if (lower == upper)
            return value >= upper ? T { 1 } : T { 0 };

        const T Amount = Clamp((value - lower) / (upper - lower));

        return Amount * Amount * (T { 3 } - T { 2 } * Amount);
    }

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

        /// <summary>
        /// 
        /// </summary>
        D2D1_COLOR_F GetThemeColor(float hue, float hueOffset, float saturation, float value, float alpha) const noexcept
        {
            value = Clamp(value * Brightness);

            if (Monochrome)
                return D2D1::ColorF(value, value, value, alpha);

            return HSVToColor(hue + (hueOffset * HueRange), saturation * Saturation, value, alpha);
        }
    };

    struct interaction_t
    {
        double X       = 0.;
        double Y       = 0.;
        double Zoom    = 1.;
        double Impulse = 0.;
    };

    class drag_helper_t
    {
    public:
        void Start(HWND hWnd, int x, int y, const EyeCandy::interaction_t & interaction) noexcept
        {
            ::SetFocus(hWnd);
            ::SetCapture(hWnd);

            _hWnd = hWnd;
            _IsDragging = true;

            _Origin = { x, y };

            _FocusX = interaction.X;
            _FocusY = interaction.Y;
        }

        void Stop(int x, int y, EyeCandy::interaction_t & interaction) noexcept
        {
            if (!_IsDragging)
                return;

            const int dx = x - _Origin.x;
            const int dy = y - _Origin.y;

            if (((dx * dx) + (dy * dy)) < 16)
                interaction.Impulse = 1.;

            _IsDragging = false;

            if (_hWnd == ::GetCapture())
                ::ReleaseCapture();

            _hWnd = NULL;
        }

        void Move(int x, int y, EyeCandy::interaction_t & interaction) noexcept
        {
            if (!_IsDragging)
                return;

            RECT rc;

            ::GetClientRect(_hWnd, &rc);

            interaction.X = _FocusX - 2.5 * (double) (x - _Origin.x) / std::max(1L, rc.right);
            interaction.Y = _FocusY - 2.5 * (double) (y - _Origin.y) / std::max(1L, rc.bottom);
        }

    private:
        HWND _hWnd = NULL;

        bool _IsDragging = false;
        POINT _Origin {};

        double _FocusX = 0.;
        double _FocusY = 0.;
    };
}
