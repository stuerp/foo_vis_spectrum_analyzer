
/** $VER: WindowFunctions.h (2026.09.05) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <stdexcept>

using namespace std;

enum class WindowFunction
{
    BoxCar = 0,         // Rectangular

    Hann,               // Cosine-squared
    Hamming,            // Raied cosine
    Blackman,
    Nuttall,
    FlatTop,

    Bartlett,           // Triangular
    Parzen,

    Welch,
    PowerOfSine,
    PowerOfCircle,

    Gaussian,
    Tukey,              // Tapered cosine
    Kaiser,
    Poisson,            // Exponential

    HyperbolicSecant,
    QuadraticSpline,

    OggVorbis,
    CascadedSine,

    Galss,              // https://hydrogenaud.io/index.php/topic,125031.msg1036200.html#msg1036200

    Lanczos,
    BlackmanHarris4,    // 4-term Blackman-Harris

    Count
};

#pragma warning(push)
#pragma warning(disable: 4820 5026 5027)

/// <summary>
/// Implements the base functor for all window functions.
/// </summary>
class window_function_t
{
public:
    window_function_t() noexcept = default;

    window_function_t(double skew, bool truncate) noexcept : _Skew(skew), _Truncate(truncate), _SkewSquared(10. * skew * skew) { }

    window_function_t(const window_function_t&) = delete;
    window_function_t& operator=(const window_function_t&) = delete;
    window_function_t(window_function_t&&) = delete;
    window_function_t& operator=(window_function_t&&) = delete;

    virtual ~window_function_t() = default;

    [[nodiscard]]
    virtual double operator()(double x) const noexcept { return 0.; };

    [[nodiscard]]
    static window_function_t * Create(WindowFunction windowFunction, double windowParameter, double windowSkew, bool truncate);

protected:
    /// <summary>
    /// Applies skew and checks whether the transformed coordinate is inside the supported interval.
    /// Returns std::nullopt when truncation is enabled and the transformed coordinate lies outside [-1, 1].
    /// </summary>
    [[nodiscard]]
    std::optional<double> Transform(double x) const noexcept
    {
        if (_Skew != 0.)
        {
            const double Normalization = 1. + _SkewSquared;

            if (_Skew > 0.)
            {
                const double y = (x * 0.5) - 0.5;
                const double Denominator = 1. - (y * _SkewSquared);

                if (Denominator == 0.)
                    return std::nullopt;

                x = ((y * Normalization) / Denominator) * 2. + 1.;
            }
            else
            {
                const double y = (x * 0.5) + 0.5;
                const double Denominator = 1. + (y * _SkewSquared);

                if (Denominator == 0.)
                    return std::nullopt;

                x = ((y * Normalization) / Denominator) * 2. - 1.;
            }
        }

        if (!std::isfinite(x))
            return std::nullopt;

        if (_Truncate && std::abs(x) > 1.)
            return std::nullopt;

        return x;
    }

private:
    double _Skew = 0.;
    bool _Truncate = true; // True when the input value of the function should be truncated to 0 if it is outside the range of [-1, 1]

    double _SkewSquared = 0.;
};

/// <summary>
/// Implements the Box Car (rectangular, Dirichlet) window function.
/// Sharpest time edges, highest sidelobes (~13 dB). Use when you need perfect time localization and don’t care about leakage.
/// </summary>
class BoxCar final : public window_function_t
{
public:
    BoxCar(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        return Transform(x).has_value() ? 1. : 0.;
    }
};

/// <summary>
/// Implements the Hann (Hanning, cosine squared, raised cosine) window function.
/// Sidelobes ~31 dB, good compromise. Use with speech, audio, general-purpose.
/// </summary>
class Hann final : public window_function_t
{
public:
    Hann(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return (1. + std::cos(*t * std::numbers::pi)) / 2.;
    }
};

/// <summary>
/// Implements the Hamming (raised cosine) window function.
/// Lower first sidelobe than Hann, but unlike Hann its endpoints do not reach zero. Suitable when reduced spectral leakage is preferred over exact endpoint continuity.
/// </summary>
class Hamming final : public window_function_t
{
public:
    Hamming(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return 0.54 + (0.46 * std::cos(*t * std::numbers::pi));
    }
};

/// <summary>
/// Implements the Blackman window function.
/// Provides stronger sidelobe suppression (~58 dB) than Hann and Hamming at the cost of a wider main lobe. Use when a high dynamic range is needed.
/// </summary>
class Blackman final : public window_function_t
{
public:
    Blackman(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Phase = *t * std::numbers::pi;

        return 0.42 + (0.5 * std::cos(Phase)) + (0.08 * std::cos(2. * Phase));
    }
};

/// <summary>
/// Implements the Nuttall window function.
/// Provides strong sidelobe suppression at the cost of a wider main lobe.
/// </summary>
class Nuttall final : public window_function_t
{
public:
    Nuttall(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Phase = *t * std::numbers::pi;

        return 0.355768 + (0.487396 * std::cos(Phase)) + (0.144232 * std::cos(2. * Phase)) + (0.012604 * std::cos(3. * Phase));
    }
};

/// <summary>
/// Implements the Flat Top window function.
/// Provides high amplitude accuracy and low scalloping loss at the cost of a significantly wider main lobe.
/// </summary>
class FlatTop final : public window_function_t
{
public:
    FlatTop(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        constexpr double A0 = 0.215578950;
        constexpr double A1 = 0.416631580;
        constexpr double A2 = 0.277263158;
        constexpr double A3 = 0.083578947;
        constexpr double A4 = 0.006947368;

        constexpr double Normalization = A0 + A1 + A2 + A3 + A4;

        const double Phase = *t * std::numbers::pi;

        const double Value = A0 + (A1 * std::cos(Phase)) + (A2 * std::cos(2. * Phase)) + (A3 * std::cos(3. * Phase)) + (A4 * std::cos(4. * Phase));

        return Value / Normalization;
    }
};

/// <summary>
/// Implements the Bartlett (Triangular) window function.
/// Reaches its maximum at the center and decreases linearly to zero at both endpoints.
/// </summary>
class Bartlett final : public window_function_t
{
public:
    Bartlett(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return 1. - std::fabs(*t);
    }
};

/// <summary>
/// Implements the Parzen window function.
/// </summary>
class Parzen final : public window_function_t
{
public:
    Parzen(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const auto u = std::fabs(*t);

        if (u <= 0.5)
            return 1. - (6. * u * u) + (6. * u * u * u);

        const auto v = 1. - u;

        return 2. * v * v * v;
    }
};

/// <summary>
/// Implements the Welch window function.
/// </summary>
class Welch final : public window_function_t
{
public:
    Welch(double skew, bool truncate, double power) noexcept : window_function_t(skew, truncate), _Power(std::max(0., power)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return std::pow(1. - *t * *t, _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Power-of-Sine window function.
/// The power controls the shape of the window. A power of 1 produces a sine window, while a power of 2 produces a Hann window.
/// </summary>
class PowerOfSine final : public window_function_t
{
public:
    PowerOfSine(double skew, bool truncate, double power) noexcept : window_function_t(skew, truncate), _Power(std::max(0., power)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Phase = *t * std::numbers::pi / 2.;

        return std::pow(std::cos(Phase), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Power-of-Circle window function.
/// The power controls the curvature of the window. A power of 1 produces the standard Welch window.
/// </summary>
class PowerOfCircle final : public window_function_t
{
public:
    PowerOfCircle(double skew, bool truncate, double power) noexcept : window_function_t(skew, truncate), _Power(std::max(0., power)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return std::pow(std::max(0., 1. - (*t * *t)), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Gaussian window function.
/// No sidelobes at all (theoretically), but infinite support. Use when you can afford a longer window.
/// Sigma controls the width of the window. Smaller positive values produce a narrower window, while larger values approach a rectangular window.
/// The Gaussian window uses windowParameter as the standard deviation, typically called sigma. On the normalized interval [-1, 1], its value is exp(-0.5 × (x / sigma)²).
/// </summary>
class Gaussian final : public window_function_t
{
public:
    Gaussian(double skew, bool truncate, double sigma) noexcept : window_function_t(skew, truncate), _Sigma(std::fabs(sigma)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        if (_Sigma == 0.)
            return (*t == 0.) ? 1. : 0.;

        const double Ratio = *t / _Sigma;

        return std::exp(-0.5 * Ratio * Ratio);
    }

private:
    double _Sigma;
};

/// <summary>
/// Implements the Tukey tapered-cosine window function.
/// Alpha controls the proportion of the window occupied by the cosine tapers. An alpha of 0 produces a BoxCar window, while an alpha of 1 produces a Hann window.
/// The Tukey window combines a flat central section with cosine-tapered edges. Here, windowParameter is the taper ratio alpha, conventionally constrained to [0, 1], where 0 produces BoxCar and 1 produces Hann.
/// </summary>
class Tukey final : public window_function_t
{
public:
    Tukey(double skew, bool truncate, double alpha) noexcept : window_function_t(skew, truncate), _Alpha(std::clamp(alpha, 0., 1.)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        if (_Alpha == 0.)
            return 1.;

        const double Absolute = std::fabs(*t);
        const double TaperStart = 1. - _Alpha;

        if (Absolute <= TaperStart)
            return 1.;

        const double Phase = std::numbers::pi * (Absolute - TaperStart) / _Alpha;

        return (1. + std::cos(Phase)) / 2.;
    }

private:
    double _Alpha;
};

/// <summary>
/// Implements the Kaiser window function.
/// Beta controls the shape of the window. A beta of zero produces a BoxCar window, while larger values increase sidelobe suppression at the cost of a wider main lobe.
/// Uses the modified Bessel function of the first kind, order zero. windowParameter serves as beta, which controls the tradeoff between main-lobe width and sidelobe suppression.
/// </summary>

class Kaiser final : public window_function_t
{
public:
    Kaiser(double skew, bool truncate, double beta) noexcept : window_function_t(skew, truncate), _Beta(std::fabs(beta)), _Normalization(std::cyl_bessel_i(0., std::fabs(beta))) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Radicand = std::max(0., 1. - *t * *t);

        return std::cyl_bessel_i(0., _Beta * std::sqrt(Radicand)) / _Normalization;
    }

private:
    double _Beta;
    double _Normalization;
};

/// <summary>
/// Implements the Poisson window function.
/// </summary>
class Poisson final : public window_function_t
{
public:
    Poisson(double skew, bool truncate, double alpha) noexcept : window_function_t(skew, truncate), _Alpha(alpha) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return std::exp(-_Alpha * std::fabs(*t));
    }

private:
    double _Alpha;
};

/// <summary>
/// Implements the Hyperbolic Secant window function.
/// Beta controls the concentration of the window. A beta of zero produces a BoxCar window, while larger values increase attenuation toward the endpoints.
/// </summary>
class HyperbolicSecant final : public window_function_t
{
public:
    HyperbolicSecant(double skew, bool truncate, double beta) noexcept : window_function_t(skew, truncate), _Beta(std::fabs(beta)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        return 1. / std::cosh(_Beta * *t);
    }

private:
    double _Beta;
};

/// <summary>
/// Implements a quadratic spline window function.
/// </summary>
class QuadraticSpline final : public window_function_t
{
public:
    QuadraticSpline(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const auto u = std::fabs(*t);

        if (u <= 0.5)
            return 1. - (2. * u * u);

        const auto v = 1. - u;

        return (2. * v * v);
    }
};

/// <summary>
/// Implements the Ogg Vorbis window function.
/// </summary>
class OggVorbis final : public window_function_t
{
public:
    OggVorbis(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Cosine = std::cos(std::numbers::pi / 2. * *t);

        return std::sin(std::numbers::pi / 2. * Cosine * Cosine);
    }
};

/// <summary>
/// Implements a cascaded sine / cosine window function.
/// </summary>
class CascadedSine final : public window_function_t
{
public:
    CascadedSine(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Sine = std::sin(*t * std::numbers::pi / 2.);

        return 1. - std::sin(std::numbers::pi / 2. * Sine * Sine);
    }
};

/// <summary>
/// Implements a Galss window function.
/// </summary>
class Galss final : public window_function_t
{
public:
    Galss(double skew, bool truncate) noexcept : window_function_t(skew, truncate), _Denominator(std::tanh(std::numbers::sqrt2) * std::tanh(std::numbers::sqrt2)) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        const double Value = *t;

        const double Shape = (1. - 1. / (Value + 2.)) * (1. - 1. / (-Value + 2.)) * 4.;

        const double Taper = -std::tanh(std::numbers::sqrt2 * (-Value + 1.)) * std::tanh(std::numbers::sqrt2 * (-Value - 1.));

        return Shape * Shape * Taper / _Denominator;
    }

private:
    double _Denominator;
}; 

/// <summary>
/// Implements a Lanczos window function.
/// </summary>
class Lanczos final : public window_function_t
{
public:
    Lanczos(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        if (std::abs(*t) < std::numeric_limits<double>::epsilon())
            return 1.;

        const auto z = std::numbers::pi * *t;

        return std::sin(z) / z;
    }
};

/// <summary>
/// Implements a 4-term Blackman-Harris window function.
/// </summary>
/// Produces very clean peaks with minimal leakage from strong tones into neighboring bins. w(x) = a0​ + a1 ​cos(πt) + a2 ​cos(2πt) + a3​ cos(3πt)
class BlackmanHarris4 final : public window_function_t
{
public:
    BlackmanHarris4(double skew, bool truncate) noexcept : window_function_t(skew, truncate) { }

    [[nodiscard]]
    double operator()(double x) const noexcept override final
    {
        const auto t = Transform(x);

        if (!t)
            return 0.;

        constexpr double a0 = 0.35875;
        constexpr double a1 = 0.48829;
        constexpr double a2 = 0.14128;
        constexpr double a3 = 0.01168;

        const auto p = std::numbers::pi * *t;

        return a0 + (a1 * std::cos(p)) + (a2 * std::cos(2. * p)) + (a3 * std::cos(3. * p));
    }
};

/// <summary>
/// Creates the specified window function.
/// </summary>
inline window_function_t * window_function_t::Create(WindowFunction windowFunction, double windowParameter, double windowSkew, bool truncate)
{
    switch (windowFunction)
    {
        case WindowFunction::BoxCar:
            return new BoxCar(windowSkew, truncate);

        case WindowFunction::Hann:
            return new Hann(windowSkew, truncate);

        case WindowFunction::Hamming:
            return new Hamming(windowSkew, truncate);

        case WindowFunction::Blackman:
            return new Blackman(windowSkew, truncate);

        case WindowFunction::Nuttall:
            return new Nuttall(windowSkew, truncate);

        case WindowFunction::FlatTop:
            return new FlatTop(windowSkew, truncate);

        case WindowFunction::Lanczos:
            return new Lanczos(windowSkew, truncate);

        case WindowFunction::BlackmanHarris4:
            return new BlackmanHarris4(windowSkew, truncate);

        /** B-spline windows **/

        case WindowFunction::Bartlett:
            return new Bartlett(windowSkew, truncate);

        case WindowFunction::Parzen:
            return new Parzen(windowSkew, truncate);

        /** Polynomial windows **/

        case WindowFunction::Welch:
            return new Welch(windowSkew, truncate, windowParameter);

        case WindowFunction::PowerOfSine:
            return new PowerOfSine(windowSkew, truncate, windowParameter);

        case WindowFunction::PowerOfCircle:
            return new PowerOfCircle(windowSkew, truncate, windowParameter);

        /** Adjustable windows **/

        case WindowFunction::Gaussian:
            return new Gaussian(windowSkew, truncate, windowParameter);

        case WindowFunction::Tukey:
            return new Tukey(windowSkew, truncate, windowParameter);

        case WindowFunction::Kaiser:
            return new Kaiser(windowSkew, truncate, windowParameter);

        case WindowFunction::Poisson:
            return new Poisson(windowSkew, truncate, windowParameter);

        // Other windows
        case WindowFunction::HyperbolicSecant:
            return new HyperbolicSecant(windowSkew, truncate, windowParameter);

        case WindowFunction::QuadraticSpline:
            return new QuadraticSpline(windowSkew, truncate);

        case WindowFunction::OggVorbis:
            return new OggVorbis(windowSkew, truncate);

        case WindowFunction::CascadedSine:
            return new CascadedSine(windowSkew, truncate);

        case WindowFunction::Galss:
            return new Galss(windowSkew, truncate);

        case WindowFunction::Count:
        default:
            throw std::invalid_argument("Unknown window function");
    }
}

#pragma warning(pop)

inline void TestWindowFunctions()
{
//  constexpr double Epsilon = 1.0e-12;
    constexpr double Epsilon = std::numeric_limits<double>::epsilon();

    {
        const BoxCar w{0., true};

        assert(w( 0.)   == 1.);
        assert(w(-1.)   == 1.);
        assert(w( 1.)   == 1.);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    {
        const Hann w{0., true};

        assert(std::abs(w( 0.) - 1.) < Epsilon);
        assert(std::abs(w(-1.))      < Epsilon);
        assert(std::abs(w( 1.))      < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Hann
    {
        const Hann w{0., false};

        const double Expected = (1. + std::cos(1.5 * std::numbers::pi)) / 2.;

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    {
        const Hamming w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.) - 0.08) < Epsilon);
        assert(std::abs(w( 1.) - 0.08) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Hamming
    {
        const Hamming w{0., false};

        const double Expected = 0.54 + (0.46 * std::cos(1.5 * std::numbers::pi));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Blackmann
    {
        const Blackman w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Blackmann
    {
        const Blackman w{0., false};

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = 0.42 + (0.5 * std::cos(Phase)) + (0.08 * std::cos(2. * Phase));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Nuttall
    {
        const Nuttall w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Nuttall
    {
        const Nuttall w{0., false};

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = 0.355768 + (0.487396 * std::cos(Phase)) + (0.144232 * std::cos(2. * Phase)) + (0.012604 * std::cos(3. * Phase));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Flat Top
    {
        constexpr double A0 = 0.215578950;
        constexpr double A1 = 0.416631580;
        constexpr double A2 = 0.277263158;
        constexpr double A3 = 0.083578947;
        constexpr double A4 = 0.006947368;

        constexpr double Normalization = A0 + A1 + A2 + A3 + A4;

        constexpr double Endpoint = (A0 - A1 + A2 - A3 + A4) / Normalization;

        const FlatTop w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.) - Endpoint) < Epsilon);
        assert(std::abs(w( 1.) - Endpoint) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Flat Top
    {
        const FlatTop w{0., false};

        constexpr double A0 = 0.215578950;
        constexpr double A1 = 0.416631580;
        constexpr double A2 = 0.277263158;
        constexpr double A3 = 0.083578947;
        constexpr double A4 = 0.006947368;

        constexpr double Normalization = A0 + A1 + A2 + A3 + A4;

        const double Phase = 1.5 * std::numbers::pi;

        const double Expected = (A0 + (A1 * std::cos(Phase)) + (A2 * std::cos(2. * Phase)) + (A3 * std::cos(3. * Phase)) + (A4 * std::cos(4. * Phase))) / Normalization;

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Bartlett
    {
        const Bartlett w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test known intermediate values.
        assert(std::abs(w(-0.25) - 0.75) < Epsilon);
        assert(std::abs(w( 0.25) - 0.75) < Epsilon);
        assert(std::abs(w(-0.75) - 0.25) < Epsilon);
        assert(std::abs(w( 0.75) - 0.25) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated Bartlett
    {
        const Bartlett w{0., false};

        const double Expected = 1. - std::abs(1.5);

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // Welch
    {
        const Welch w(0., true, 1.);

        // Test maximum at the center.
        assert(std::fabs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::fabs(w(-1.)) < Epsilon);
        assert(std::fabs(w( 1.)) < Epsilon);

        // Test known intermediate values.
        assert(std::fabs(w(-0.5) - 0.75) < Epsilon);
        assert(std::fabs(w( 0.5) - 0.75) < Epsilon);

        assert(std::fabs(w(-0.25) - 0.9375) < Epsilon);
        assert(std::fabs(w( 0.25) - 0.9375) < Epsilon);

        // Test symmetry around zero.
        assert(std::fabs(w(-0.125) - w(0.125)) < Epsilon);
        assert(std::fabs(w(-0.625) - w(0.625)) < Epsilon);
        assert(std::fabs(w(-0.875) - w(0.875)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);

        assert(w(-2.) == 0.);
        assert(w( 2.) == 0.);
    }

    {
        const Welch w(0., false, 1.);

        assert(std::fabs(w(-1.5) + 1.25) < Epsilon);
        assert(std::fabs(w( 1.5) + 1.25) < Epsilon);

        assert(std::fabs(w(-2.) + 3.) < Epsilon);
        assert(std::fabs(w( 2.) + 3.) < Epsilon);
    }

    // Tukey
    {
        const Tukey w(0., true, 0.5);

        // Test maximum at the center.
        assert(std::fabs(w(0.) - 1.) < Epsilon);

        // Test central region.
        assert(std::fabs(w(-0.5)  - 1.) < Epsilon);
        assert(std::fabs(w(-0.25) - 1.) < Epsilon);
        assert(std::fabs(w( 0.25) - 1.) < Epsilon);
        assert(std::fabs(w( 0.5)  - 1.) < Epsilon);

        // Test endpoints.
        assert(std::fabs(w(-1.)) < Epsilon);
        assert(std::fabs(w( 1.)) < Epsilon);

        // Halfway through each taper, the value is 0.5.
        assert(std::fabs(w(-0.75) - 0.5) < Epsilon);
        assert(std::fabs(w( 0.75) - 0.5) < Epsilon);

        // Verify another known value in the taper.
        const double Expected = (1. + std::cos(std::numbers::pi / 2.)) / 2.;

        assert(std::fabs(w(0.75) - Expected) < Epsilon);

        // The non-skewed Tukey window is symmetric.
        constexpr double Samples[]
        {
            0.,
            0.125,
            0.25,
            0.5,
            0.625,
            0.75,
            0.875,
            1.
        };

        for (const double x : Samples)
            assert(std::fabs(w(-x) - w(x)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
        assert(w(-2.)   == 0.);
        assert(w( 2.)   == 0.);
    }

    // Ogg Vorbis
    {
        const OggVorbis w{0., true};

        assert(std::abs(w( 0.) - 1.) < Epsilon);
        assert(std::abs(w(-1.))      < Epsilon);
        assert(std::abs(w( 1.))      < Epsilon);

        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Cascaded sine
    {
        const CascadedSine w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        assert(std::abs(w(-1.)) < Epsilon);
        assert(std::abs(w( 1.)) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated cascaded sine
    {
        const CascadedSine w{0., false};

        const double HalfPi = std::numbers::pi / 2.;

        const double Sine = std::sin(1.5 * HalfPi);

        const double Expected = 1. - std::sin(HalfPi * Sine * Sine);

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }

    // 4-term Blackman-Harris
    {
        const BlackmanHarris4 w{0., true};

        // Test maximum at the center.
        assert(std::abs(w(0.) - 1.) < Epsilon);

        // Test endpoints.
        constexpr double Endpoint = 0.00006;

        assert(std::abs(w(-1.) - Endpoint) < Epsilon);
        assert(std::abs(w( 1.) - Endpoint) < Epsilon);

        // Test symmetry around zero.
        assert(std::abs(w(-0.25) - w(0.25)) < Epsilon);
        assert(std::abs(w(-0.75) - w(0.75)) < Epsilon);

        // Test truncation outside the supported interval.
        assert(w(-1.01) == 0.);
        assert(w( 1.01) == 0.);
    }

    // Non-truncated 4-term Blackman-Harris
    {
        const BlackmanHarris4 w{0., false};

        constexpr double a0 = 0.35875;
        constexpr double a1 = 0.48829;
        constexpr double a2 = 0.14128;
        constexpr double a3 = 0.01168;

        const double t = 1.5;

        const double Expected = a0 + (a1 * std::cos(std::numbers::pi * t)) + (a2 * std::cos(2. * std::numbers::pi * t)) + (a3 * std::cos(3. * std::numbers::pi * t));

        assert(std::abs(w(1.5) - Expected) < Epsilon);
    }
}
