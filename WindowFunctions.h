
/** $VER: WindowFunctions.h (2023.12.03) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "Configuration.h"

using namespace std;

enum class WindowFunctions
{
    Boxcar = 0,

    Hann,
    Hamming,
    Blackman,
    Nuttall,
    FlatTop,

    Bartlett,
    Parzen,

    Welch,
    PowerOfSine,

    Gauss,
    Tukey,
    Kaiser,
    Poison,

    HyperbolicSecant,
    QuadraticSpline
};

/// <summary>
/// Implements the base functor for all window functions.
/// </summary>
class WindowFunction
{
public:
    WindowFunction() : _Skew(), _Truncate(true) { }

    WindowFunction(const WindowFunction &) = delete;
    WindowFunction & operator=(const WindowFunction & other) = delete;
    WindowFunction(WindowFunction &&) = delete;
    WindowFunction & operator=(WindowFunction && other) = delete;

    virtual ~WindowFunction() { }

    WindowFunction(double skew, bool truncate) : _Skew(skew), _Truncate(truncate) { _SkewSquared = 10. * _Skew * _Skew; }

    virtual double operator () (double x) const
    {
        x = (_Skew > 0.) ? ((x / 2. - 0.5) / (1. - (x / 2. - 0.5) * _SkewSquared)) / (1. / (1. + _SkewSquared)) * 2. + 1.:
                           ((x / 2. + 0.5) / (1. + (x / 2. + 0.5) * _SkewSquared)) / (1. / (1. + _SkewSquared)) * 2. - 1.;

        return (_Truncate && (::fabs(x) >  1.)) ? 0. : x;
    }

    static WindowFunction * Create(WindowFunctions windowFunction, double windowParameter, double windowSkew, bool truncate);

private:
    double _Skew;
    bool _Truncate;

    double _SkewSquared;
};

/// <summary>
/// Implements the boxcar (rectangular, Dirichlet) window function.
/// </summary>
class Boxcar : public WindowFunction
{
public:
    Boxcar(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Boxcar() { }

    virtual double operator () (double) const override
    {
        return __super::operator()(1.);
    }
};

/// <summary>
/// Implements the Hanning (Cosine squared) window function.
/// </summary>
class Hann : public WindowFunction
{
public:
    Hann(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Hann() { }

    virtual double operator () (double x) const override
    {
        return ::pow(::cos(__super::operator()(x) * M_PI_2), 2.);
    //  return 0.5 * (1. - ::cos(x * 2. * M_PI));
    }
};

/// <summary>
/// Implements the Hamming window function.
/// </summary>
class Hamming : public WindowFunction
{
public:
    Hamming(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Hamming() { }

    virtual double operator () (double x) const override
    {
        return 0.53836 - (0.46164 * ::cos(__super::operator()(x) * M_PI_2));
    //  return 0.54      + (0.46 * ::cos(x * M_PI));
    //  return (25./46.) + (0.46 * ::cos(x * M_PI));
    }
};

/// <summary>
/// Implements the Blackman window function.
/// </summary>
class Blackman : public WindowFunction
{
public:
    Blackman(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Blackman() { }

    virtual double operator () (double x) const override
    {
        return 0.42 + (0.5 * ::cos(x * M_PI)) + (0.08 * ::cos(2. * x * M_PI));
    }
};

/// <summary>
/// Implements the Nuttall window function.
/// </summary>
class Nuttall : public WindowFunction
{
public:
    Nuttall(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Nuttall() { }

    virtual double operator () (double x) const override
    {
        return 0.355768 + (0.487396 * ::cos(x * M_PI)) + (0.144232 * ::cos(2. * x * M_PI)) + (0.012604 * ::cos(3. * x * M_PI));
    }
};

/// <summary>
/// Implements the Flat Top window function.
/// </summary>
class FlatTop : public WindowFunction
{
public:
    FlatTop(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~FlatTop() { }

    virtual double operator () (double x) const override
    {
        return 0.21557895 + (0.41663158 * ::cos(x * M_PI)) + (0.277263158 * ::cos(2. * x * M_PI)) + (0.083578947 * ::cos(3. * x * M_PI)) + (0.006947368 * ::cos(4. * x * M_PI));
    }
};

/// <summary>
/// Implements the Bartlett window function.
/// </summary>
class Bartlett : public WindowFunction
{
public:
    Bartlett(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Bartlett() { }

    virtual double operator () (double x) const override
    {
        return 1. - ::fabs(__super::operator()(x));
    }
};

/// <summary>
/// Implements the Parzen window function.
/// </summary>
class Parzen : public WindowFunction
{
public:
    Parzen(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Parzen() { }

    virtual double operator () (double x) const override
    {
        return (::fabs(x) > 0.5) ? (-2. * ::pow((-1. + ::fabs(x)), 3.)) : (1. - 24. * ::pow(::fabs(x / 2.), 2.) + 48. * ::pow(::fabs(x / 2.), 3.));
    }
};

/// <summary>
/// Implements the Welch window function.
/// </summary>
class Welch : public WindowFunction
{
public:
    Welch(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~Welch() { }

    virtual double operator () (double x) const override
    {
        return 1. - (x * x);
    }
};

/// <summary>
/// Implements the Power-of-Sine window function.
/// </summary>
class PowerOfSine : public WindowFunction
{
public:
    PowerOfSine(double skew, bool truncate, double power) : WindowFunction(skew, truncate), _Power(power) { }

    virtual ~PowerOfSine() { }

    virtual double operator () (double x) const override
    {
        return ::pow(::cos(__super::operator()(x) * M_PI_2), _Power);
    }

private:
    double _Power;
};

/// <summary>
/// Implements the Gauss window function.
/// </summary>
class Gauss : public WindowFunction
{
public:
    Gauss(double skew, bool truncate, double sigma) : WindowFunction(skew, truncate), _Sigma(sigma) { }

    virtual ~Gauss() { }

    virtual double operator () (double x) const override
    {
        return ::exp(-(_Sigma * _Sigma) * (x * x));
    }

private:
    double _Sigma;
};

/// <summary>
/// Implements the Tukey window function.
/// </summary>
class Tukey : public WindowFunction
{
public:
    Tukey(double skew, bool truncate, double parameter) : WindowFunction(skew, truncate), _Parameter(parameter) { }

    virtual ~Tukey() { }

    virtual double operator () (double x) const override
    {
        return (::fabs(x) <= 1. - _Parameter) ? 1 : (x > 0. ? ::pow(-::sin((x - 1.) * M_PI / _Parameter / 2.), 2.) : ::pow(::sin((x + 1.) * M_PI / _Parameter / 2.), 2.));
    }

private:
    double _Parameter;
};

/// <summary>
/// Implements the Kaiser window function.
/// </summary>
class Kaiser : public WindowFunction
{
public:
    Kaiser(double skew, bool truncate, double alpha) : WindowFunction(skew, truncate), _AlphaSquared(alpha * alpha) { }

    virtual ~Kaiser() { }

    virtual double operator () (double x) const override
    {
        return ::cosh(::sqrt(1. - (x * x)) * _AlphaSquared) / ::cosh(_AlphaSquared);
    }

private:
    double _AlphaSquared;
};

/// <summary>
/// Implements the Poison window function.
/// </summary>
class Poison : public WindowFunction
{
public:
    Poison(double skew, bool truncate, double parameter) : WindowFunction(skew, truncate), _ParameterSquared(parameter * parameter) { }

    virtual ~Poison() { }

    virtual double operator () (double x) const override
    {
        return ::exp(-::fabs(x * _ParameterSquared));
    }

private:
    double _ParameterSquared;
};

/// <summary>
/// Implements a hyperbolic secant window function.
/// </summary>
class HyperbolicSecant : public WindowFunction
{
public:
    HyperbolicSecant(double skew, bool truncate, double parameter) : WindowFunction(skew, truncate), _ParameterSquared(parameter * parameter) { }

    virtual ~HyperbolicSecant() { }

    virtual double operator () (double x) const override
    {
        return 1. / ::cosh(x * _ParameterSquared);
    }

private:
    double _ParameterSquared;
};

/// <summary>
/// Implements a quadratic spline window function.
/// </summary>
class QuadraticSpline : public WindowFunction
{
public:
    QuadraticSpline(double skew, bool truncate) : WindowFunction(skew, truncate) { }

    virtual ~QuadraticSpline() { }

    virtual double operator () (double x) const override
    {
        return (::fabs(x) <= 0.5) ? -::pow((x * M_SQRT2), 2.) + 1. : ::pow(::fabs(x * M_SQRT2) - M_SQRT2, 2.);
    }
};

/// <summary>
/// Creates the specified window function.
/// </summary>
inline WindowFunction * WindowFunction::Create(WindowFunctions windowFunction, double windowParameter, double windowSkew, bool truncate)
{
    switch (windowFunction)
    {
        default:

        case WindowFunctions::Boxcar:
            return new Boxcar(windowSkew, truncate);

        // Cosine-sum windows
        case WindowFunctions::Hann:
            return new Hann(windowSkew, truncate);

        case WindowFunctions::Hamming:
            return new Hamming(windowSkew, truncate);

        case WindowFunctions::Blackman:
            return new Blackman(windowSkew, truncate);

        case WindowFunctions::Nuttall:
            return new Nuttall(windowSkew, truncate);

        case WindowFunctions::FlatTop:
            return new FlatTop(windowSkew, truncate);

        // B-spline windows
        case WindowFunctions::Bartlett: // Triangular
            return new Bartlett(windowSkew, truncate);

        case WindowFunctions::Parzen:
            return new Parzen(windowSkew, truncate);

        // Polynomial windows
        case WindowFunctions::Welch:
            return new Welch(windowSkew, truncate);

        case WindowFunctions::PowerOfSine:
            return new PowerOfSine(windowSkew, truncate, windowParameter);

        // Adjustable windows
        case WindowFunctions::Gauss:
            return new Gauss(windowSkew, truncate, windowParameter);

        case WindowFunctions::Tukey:
            return new Tukey(windowSkew, truncate, windowParameter);

        case WindowFunctions::Kaiser:
            return new Kaiser(windowSkew, truncate, windowParameter);

        case WindowFunctions::Poison:
            return new Poison(windowSkew, truncate, windowParameter);

        // Other windows
        case WindowFunctions::HyperbolicSecant:
            return new HyperbolicSecant(windowSkew, truncate, windowParameter);

        case WindowFunctions::QuadraticSpline:
            return new QuadraticSpline(windowSkew, truncate);
    }
}
