
/** $VER: AmplitudeScaler.h (2025.10.07) P. Stuer - Implements an amplitude scaler as a functor **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include <cmath>

/// <summary>
/// Implements an amplitude scaler as a functor.
/// </summary>
class amplitude_scaler_t
{
public:
    explicit amplitude_scaler_t()
    {
        SetNormalizedMode();
    }

    inline void SetNormalizedMode() noexcept
    {
        _ScaleFunction = &amplitude_scaler_t::ScaleNormalized;
    }

    inline void SetDecibelMode(double dBMin, double dBMax) noexcept
    {
        _ScaleFunction = &amplitude_scaler_t::ScaleDBFS;

        _dBMin   = dBMin;
        _dBMax   = dBMax;
        _dBRange = _dBMax - _dBMin;
    }

    inline void SetLinearMode(double dBMin, double dBMax, double gamma, bool useAbsolute) noexcept
    {
        _ScaleFunction = &amplitude_scaler_t::ScaleLinear;

        _Gamma       = gamma;
        _UseAbsolute = useAbsolute;

        _Exponent = 1. / _Gamma;
        _RootMin  = _UseAbsolute ? 0. : ::pow(ToMagnitude(dBMin), _Exponent);
        _RootMax  = ::pow(ToMagnitude(dBMax), _Exponent);
    }

    /// <summary>
    /// Scales the supplied value.
    /// </summary>
    inline double operator()(double value) const noexcept
    {
        return (this->*_ScaleFunction)(value);
    }

private:
    /// <summary>
    /// Normalized scaling (no-op)
    /// </summary>
    inline double ScaleNormalized(double value) const noexcept
    {
        return value;
    }

    /// <summary>
    /// Normalized dBFS scaling. [0, 1]
    /// </summary>
    inline double ScaleDBFS(double value) const noexcept
    {
        if (value == 0.)
            return 0.;

        value = 20. * std::log10(std::abs(value));

        if (value > _dBMax)
            return 1.;

        if (value < _dBMin)
            return 0.;

        return ((value - _dBMin) / _dBRange);
    }

    /// <summary>
    /// Linear scaling
    /// </summary>
    inline double ScaleLinear(double value) const noexcept
    {
        return msc::Map(::pow(std::abs(value), _Exponent), _RootMin, _RootMax, 0., 1.);
    }

private:
    double (amplitude_scaler_t:: * _ScaleFunction)(double) const noexcept;

    double _dBMin;
    double _dBMax;
    double _dBRange;

    double _Gamma;
    bool _UseAbsolute;

    double _Exponent;
    double _RootMin;
    double _RootMax;
};
