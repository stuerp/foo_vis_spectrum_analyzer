
/** $VER: Spectrum.h (2023.12.28) P. Stuer - Represents and renders the spectrum. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4211 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"
#include "Support.h"
#include "Configuration.h"

#include "FrequencyBand.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the visualisation of the spectrum.
/// </summary>
class Spectrum
{
public:
    Spectrum() : _Configuration() {}

    Spectrum(const Spectrum &) = delete;
    Spectrum & operator=(const Spectrum &) = delete;
    Spectrum(Spectrum &&) = delete;
    Spectrum & operator=(Spectrum &&) = delete;

    void Initialize(const Configuration * configuration, CComPtr<ID2D1Factory> & direct2DFactory);

    void Move(const D2D1_RECT_F & rect);

    void Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate);

    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget);
    void ReleaseDeviceSpecificResources();

    FLOAT GetLeft() const { return _Rect.left; }
    FLOAT GetRight() const { return _Rect.right; }

private:
    HRESULT CreateGradientBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget);
    HRESULT CreatePatternBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget);
    HRESULT CreateSpline(const std::vector<FrequencyBand> & frequencyBands, double sampleRate);

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops);

private:
    const Configuration * _Configuration;

    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    D2D1_RECT_F _Rect;

    // Device-independent resources
    CComPtr<ID2D1Factory> _Direct2DFactory;
    CComPtr<ID2D1PathGeometry> _Spline;

    std::vector<D2D1_GRADIENT_STOP> _GradientStops;

    // Device-dependent resources
    CComPtr<ID2D1SolidColorBrush> _SolidBrush;
    CComPtr<ID2D1SolidColorBrush> _BackBrush;
    CComPtr<ID2D1SolidColorBrush> _WhiteBrush;

    CComPtr<ID2D1LinearGradientBrush> _GradientBrush;
    CComPtr<ID2D1BitmapBrush> _PatternBrush;
 };
