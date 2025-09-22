
/** $VER: WindowFunctions.h (2024.03.09) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <SDKDDKVer.h>
#include <Windows.h>

#include <cmath>

using namespace std;

enum class WindowFunction
{
    Boxcar = 0,         // Rectangular

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

    Gauss,
    Tukey,              // Tapered cosine
    Kaiser,
    Poison,             // Exponential

    HyperbolicSecant,
    QuadraticSpline,

    OggVorbis,
    CascadedSine,

    Galss,              // https://hydrogenaud.io/index.php/topic,125031.msg1036200.html#msg1036200

    Count
};

/// <summary>
/// Implements the base functor for all window functions.
/// </summary>
#pragma warning(disable: 4820)
class window_function_t
{
public:
    window_function_t() : _Skew(), _Truncate(true) { }

    window_function_t(const window_function_t &) = delete;
    window_function_t & operator=(const window_function_t & other) = delete;
    window_function_t(window_function_t &&) = delete;
    window_function_t & operator=(window_function_t && other) = delete;

    virtual ~window_function_t() { }

    window_function_t(double skew, bool truncate) : _Skew(skew), _Truncate(truncate) { _SkewSquared = 10. * _Skew * _Skew; }

    virtual double operator () (double x) const
    {
        x = (_Skew > 0.) ? ((x / 2. - 0.5) / (1. - (x / 2. - 0.5) * _SkewSquared)) / (1. / (1. + _SkewSquared)) * 2. + 1.:
                           ((x / 2. + 0.5) / (1. + (x / 2. + 0.5) * _SkewSquared)) / (1. / (1. + _SkewSquared)) * 2. - 1.;

        return (_Truncate && (::fabs(x) >  1.)) ? 0. : x;
    }

    static window_function_t * Create(WindowFunction windowFunction, double windowParameter, double windowSkew, bool truncate);

private:
    double _Skew;
    bool _Truncate;

    double _SkewSquared;
};

/// <summary>
/// Implements the boxcar (rectangular, Dirichlet) window function.
/// </summary>
class Boxcar : public window_function_t
{
public:
    Boxcar(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Boxcar() { }

    virtual double operator () (double) const override
    {
        return __super::operator()(1.);
    }
};

/// <summary>
/// Implements the Hann (cosine squared, raised cosine) window function.
/// </summary>
class Hann : public window_function_t
{
public:
    Hann(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Hann() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::pow(::cos(x * M_PI_2), 2.);
    //  return 0.5 * (1. - ::cos(x * 2. * M_PI));
    }
};

/// <summary>
/// Implements the Hamming window function.
/// </summary>
class Hamming : public window_function_t
{
public:
    Hamming(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Hamming() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 0.53836 - (0.46164 * ::cos(x * M_PI_2));
    //  return 0.54      + (0.46 * ::cos(x * M_PI));
    //  return (25./46.) + (0.46 * ::cos(x * M_PI));
    }
};

/// <summary>
/// Implements the Blackman window function.
/// </summary>
class Blackman : public window_function_t
{
public:
    Blackman(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Blackman() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 0.42 + (0.5 * ::cos(x * M_PI)) + (0.08 * ::cos(2. * x * M_PI));
    }
};

/// <summary>
/// Implements the Nuttall window function.
/// </summary>
class Nuttall : public window_function_t
{
public:
    Nuttall(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Nuttall() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 0.355768 + (0.487396 * ::cos(x * M_PI)) + (0.144232 * ::cos(2. * x * M_PI)) + (0.012604 * ::cos(3. * x * M_PI));
    }
};

/// <summary>
/// Implements the Flat Top window function.
/// </summary>
class FlatTop : public window_function_t
{
public:
    FlatTop(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~FlatTop() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 0.21557895 + (0.41663158 * ::cos(x * M_PI)) + (0.277263158 * ::cos(2. * x * M_PI)) + (0.083578947 * ::cos(3. * x * M_PI)) + (0.006947368 * ::cos(4. * x * M_PI));
    }
};

/// <summary>
/// Implements the Bartlett window function.
/// </summary>
class Bartlett : public window_function_t
{
public:
    Bartlett(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Bartlett() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 1. - ::fabs(x);
    }
};

/// <summary>
/// Implements the Parzen window function.
/// </summary>
class Parzen : public window_function_t
{
public:
    Parzen(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~Parzen() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return (::fabs(x) > 0.5) ? (-2. * ::pow((-1. + ::fabs(x)), 3.)) : (1. - 24. * ::pow(::fabs(x / 2.), 2.) + 48. * ::pow(::fabs(x / 2.), 3.));
    }
};

/// <summary>
/// Implements the Welch window function.
/// </summary>
class Welch : public window_function_t
{
public:
    Welch(double skew, bool truncate, double power) : window_function_t(skew, truncate), _Power(power) { }

    virtual ~Welch() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::pow(1. - (x * x), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Power-of-Sine window function.
/// </summary>
class PowerOfSine : public window_function_t
{
public:
    PowerOfSine(double skew, bool truncate, double power) : window_function_t(skew, truncate), _Power(power) { }

    virtual ~PowerOfSine() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::pow(::cos(x * M_PI_2), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Power-of-Circle window function.
/// </summary>
class PowerOfCircle : public window_function_t
{
public:
    PowerOfCircle(double skew, bool truncate, double power) : window_function_t(skew, truncate), _Power(power) { }

    virtual ~PowerOfCircle() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::pow(::sqrt(1. - (x * x)), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Gauss window function.
/// </summary>
class Gauss : public window_function_t
{
public:
    Gauss(double skew, bool truncate, double sigma) : window_function_t(skew, truncate), _Sigma(sigma) { }

    virtual ~Gauss() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::exp(-(_Sigma * _Sigma) * (x * x));
    }

private:
    double _Sigma;
};

/// <summary>
/// Implements the Tukey window function.
/// </summary>
class Tukey : public window_function_t
{
public:
    Tukey(double skew, bool truncate, double parameter) : window_function_t(skew, truncate), _Parameter(parameter) { }

    virtual ~Tukey() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return (::fabs(x) <= 1. - _Parameter) ? 1 : (x > 0. ? ::pow(-::sin((x - 1.) * M_PI / _Parameter / 2.), 2.) : ::pow(::sin((x + 1.) * M_PI / _Parameter / 2.), 2.));
    }

private:
    double _Parameter;
};

/// <summary>
/// Implements the Kaiser window function.
/// </summary>
class Kaiser : public window_function_t
{
public:
    Kaiser(double skew, bool truncate, double alpha) : window_function_t(skew, truncate), _AlphaSquared(alpha * alpha) { }

    virtual ~Kaiser() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::cosh(::sqrt(1. - (x * x)) * _AlphaSquared) / ::cosh(_AlphaSquared);
    }

private:
    double _AlphaSquared;
};

/// <summary>
/// Implements the Poison window function.
/// </summary>
class Poison : public window_function_t
{
public:
    Poison(double skew, bool truncate, double parameter) : window_function_t(skew, truncate), _ParameterSquared(parameter * parameter) { }

    virtual ~Poison() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::exp(-::fabs(x * _ParameterSquared));
    }

private:
    double _ParameterSquared;
};

/// <summary>
/// Implements a hyperbolic secant window function.
/// </summary>
class HyperbolicSecant : public window_function_t
{
public:
    HyperbolicSecant(double skew, bool truncate, double parameter) : window_function_t(skew, truncate), _ParameterSquared(parameter * parameter) { }

    virtual ~HyperbolicSecant() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 1. / ::cosh(x * _ParameterSquared);
    }

private:
    double _ParameterSquared;
};

/// <summary>
/// Implements a quadratic spline window function.
/// </summary>
class QuadraticSpline : public window_function_t
{
public:
    QuadraticSpline(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~QuadraticSpline() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return (::fabs(x) <= 0.5) ? -::pow((x * M_SQRT2), 2.) + 1. : ::pow(::fabs(x * M_SQRT2) - M_SQRT2, 2.);
    }
};

/// <summary>
/// Implements the Ogg Vorbis window function.
/// </summary>
class OggVorbis : public window_function_t
{
public:
    OggVorbis(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~OggVorbis() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::sin(M_PI_2 * ::pow(::cos(x * M_PI_2), 2.));
    }
};

/// <summary>
/// Implements a cascaded sine / cosine window function.
/// </summary>
class CascadedSine : public window_function_t
{
public:
    CascadedSine(double skew, bool truncate) : window_function_t(skew, truncate) { }

    virtual ~CascadedSine() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return 1. - ::sin(M_PI_2 * ::pow(::sin(x * M_PI_2), 2.));
    }
};

/// <summary>
/// Implements a Galss window function.
/// </summary>
class Galss : public window_function_t
{
public:
    Galss(double skew, bool truncate) : window_function_t(skew, truncate) { _Denominator = ::pow(::tanh(M_SQRT2), 2.); }

    virtual ~Galss() { }

    virtual double operator () (double x) const override
    {
        x = __super::operator()(x);

        return ::pow(((1. - 1. /(x + 2.)) * (1. - 1. / (-x + 2.))) * 4., 2.) * -(::tanh(M_SQRT2 * (-x + 1.)) * ::tanh(M_SQRT2 * (-x - 1.))) / _Denominator;
    }

private:
    double _Denominator;
};

/// <summary>
/// Creates the specified window function.
/// </summary>
inline window_function_t * window_function_t::Create(WindowFunction windowFunction, double windowParameter, double windowSkew, bool truncate)
{
    switch (windowFunction)
    {
        default:

        case WindowFunction::Boxcar:
            return new Boxcar(windowSkew, truncate);

        // Cosine-sum windows
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

        // B-spline windows
        case WindowFunction::Bartlett:
            return new Bartlett(windowSkew, truncate);

        case WindowFunction::Parzen:
            return new Parzen(windowSkew, truncate);

        // Polynomial windows
        case WindowFunction::Welch:
            return new Welch(windowSkew, truncate, windowParameter);

        case WindowFunction::PowerOfSine:
            return new PowerOfSine(windowSkew, truncate, windowParameter);

        case WindowFunction::PowerOfCircle:
            return new PowerOfSine(windowSkew, truncate, windowParameter);

        // Adjustable windows
        case WindowFunction::Gauss:
            return new Gauss(windowSkew, truncate, windowParameter);

        case WindowFunction::Tukey:
            return new Tukey(windowSkew, truncate, windowParameter);

        case WindowFunction::Kaiser:
            return new Kaiser(windowSkew, truncate, windowParameter);

        case WindowFunction::Poison:
            return new Poison(windowSkew, truncate, windowParameter);

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
            return nullptr;
    }
}
