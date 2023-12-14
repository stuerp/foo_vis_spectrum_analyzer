
/** $VER: Spectrum.h (2023.12.11) P. Stuer - Represents and renders the spectrum. **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4100 4211 4625 4626 4710 4711 5045 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include "FrequencyBand.h"
#include "Configuration.h"
#include "Math.h"

#include <vector>
#include <string>

/// <summary>
/// Implements the visualisation of the spectrum.
/// </summary>
class Spectrum
{
public:
    Spectrum() {}

    Spectrum(const Spectrum &) = delete;
    Spectrum & operator=(const Spectrum &) = delete;
    Spectrum(Spectrum &&) = delete;
    Spectrum & operator=(Spectrum &&) = delete;

    void Initialize(const Configuration * configuration)
    {
        _Configuration = configuration;

        SetGradientStops(_Configuration->_GradientStops);
        SetDrawBandBackground(_Configuration->_DrawBandBackground);

        ReleaseDeviceSpecificResources();
    }

    void Resize(const D2D1_RECT_F & rect)
    {
        _Rect = rect;
    }

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops)
    {
        _GradientStops = gradientStops;

        _GradientBrush.Release();
    }

    void SetDrawBandBackground(bool drawBandBackground)
    {
        _DrawBandBackground = drawBandBackground;
    }

    FLOAT GetLeft() const { return _Rect.left; }

    FLOAT GetRight() const { return _Rect.right; }

    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate, const Configuration & configuration);
    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget);
    HRESULT CreatePatternBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget);

    void ReleaseDeviceSpecificResources();

private:
    const Configuration * _Configuration;

    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    D2D1_RECT_F _Rect;

    bool _DrawBandBackground;
    std::vector<D2D1_GRADIENT_STOP> _GradientStops;

    CComPtr<ID2D1SolidColorBrush> _WhiteBrush;

    CComPtr<ID2D1SolidColorBrush> _BackBrush;
    CComPtr<ID2D1LinearGradientBrush> _GradientBrush;
    CComPtr<ID2D1SolidColorBrush> _ForeBrush;

    CComPtr<ID2D1BitmapBrush> _PatternBrush;
};
