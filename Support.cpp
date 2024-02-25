
/** $VER: Support.cpp (2024.02.25) P. Stuer **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Direct2D.h"

#include "SafeModuleHandle.h"
#include "Support.h"

#pragma hdrstop

/// <summary>
/// Gets the DPI setting of the specified window.
/// </summary>
HRESULT GetDPI(HWND hWnd, UINT & dpi)
{
    SafeModuleHandle Module = SafeModuleHandle(L"user32.dll");

    typedef UINT (WINAPI * GetDpiForWindow_t)(_In_ HWND hwnd);

    GetDpiForWindow_t GetDpiForWindow_ = (GetDpiForWindow_t) Module.GetFunctionAddress("GetDpiForWindow");

    if (GetDpiForWindow_ != nullptr)
        dpi = GetDpiForWindow_(hWnd);
    else
    {
        FLOAT DPIX, DPIY;

        #pragma warning(disable: 4996)
        _Direct2D.Factory->GetDesktopDpi(&DPIX, &DPIY);
        #pragma warning(default: 4996)

        dpi = (UINT) DPIX;
    }

    return S_OK;
}

/// <summary>
/// Calculates the scale factor of the specified frequency.
/// </summary>
double ScaleF(double f, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return f;

        case ScalingFunction::Logarithmic:
            return ::log2(f);

        case ScalingFunction::ShiftedLogarithmic:
            return ::log2(::pow(10, skewFactor * 4.0) + f);

        case ScalingFunction::Mel:
            return ::log2(1.0 + f / 700.0);

        case ScalingFunction::Bark: // "Critical bands"
            return (26.81 * f) / (1960.0 + f) - 0.53;

        case ScalingFunction::AdjustableBark:
            return (26.81 * f) / (::pow(10, skewFactor * 4.0) + f);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return ::log2(1.0 + 0.00437 * f);

        case ScalingFunction::Cams:
            return ::log2((f / 1000.0 + 0.312) / (f / 1000.0 + 14.675));

        case ScalingFunction::HyperbolicSine:
            return ::asinh(f / ::pow(10, skewFactor * 4));

        case ScalingFunction::NthRoot:
            return ::pow(f, (1.0 / (11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::exp2(-f / ::exp2(7 + skewFactor * 8));

        case ScalingFunction::Period:
            return 1.0 / f;
    }
}

/// <summary>
/// Calculates the frequency of the specified scale factor.
/// </summary>
double DeScaleF(double x, ScalingFunction function, double skewFactor)
{
    switch (function)
    {
        default:

        case ScalingFunction::Linear:
            return x;

        case ScalingFunction::Logarithmic:
            return ::exp2(x);

        case ScalingFunction::ShiftedLogarithmic:
            return ::exp2(x) - ::pow(10.0, skewFactor * 4.0);

        case ScalingFunction::Mel:
            return 700.0 * (::exp2(x) - 1.0);

        case ScalingFunction::Bark: // "Critical bands"
            return 1960.0 / (26.81 / (x + 0.53) - 1.0);

        case ScalingFunction::AdjustableBark:
            return ::pow(10.0, (skewFactor * 4.0)) / (26.81 / x - 1.0);

        case ScalingFunction::ERB: // Equivalent Rectangular Bandwidth
            return (1 / 0.00437) * (::exp2(x) - 1);

        case ScalingFunction::Cams:
            return (14.675 * ::exp2(x) - 0.312) / (1.0 - ::exp2(x)) * 1000.0;

        case ScalingFunction::HyperbolicSine:
            return ::sinh(x) * ::pow(10.0, skewFactor * 4);

        case ScalingFunction::NthRoot:
            return ::pow(x, ((11.0 - skewFactor * 10.0)));

        case ScalingFunction::NegativeExponential:
            return -::log2(-x) * ::exp2(7.0 + skewFactor * 8.0);

        case ScalingFunction::Period:
            return 1.0 / x;
    }
}

/// <summary>
/// 
/// </summary>
double LogSpace(double minFreq, double maxFreq, double bandIndex, size_t maxBands, double skewFactor)
{
    const double CenterFreq = minFreq * ::pow((maxFreq / minFreq), (bandIndex / (double) maxBands));

    return CenterFreq * (1 - skewFactor) + (minFreq + ((maxFreq - minFreq) * bandIndex * (1. / (double) maxBands))) * skewFactor;
}
