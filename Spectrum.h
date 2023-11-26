
/** $VER: Spectrum.h (2023.11.26) P. Stuer - Represents and renders the spectrum. **/

#pragma once

#include "framework.h"

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

        ReleaseDeviceSpecificResources();
    }

    void Resize(const D2D1_RECT_F & rect)
    {
        _Rect = rect;
    }

    void SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops)
    {
        _GradientStops = gradientStops;

        _ForegroundBrush.Release();
    }

    void SetDrawBandBackground(bool drawBandBackground)
    {
        _DrawBandBackground = drawBandBackground;
    }

    /// <summary>
    /// Renders this instance to the specified render target.
    /// </summary>
    HRESULT Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate, const Configuration & configuration)
    {
        CreateDeviceSpecificResources(renderTarget);

        const FLOAT Width = _Rect.right - _Rect.left;
        const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

        FLOAT x1 = _Rect.left;// + (Width - ((FLOAT) frequencyBands.size() * BandWidth)) / 2.f;
        FLOAT x2 = x1 + BandWidth;

        const FLOAT y1 = _Rect.top;
        const FLOAT y2 = _Rect.bottom - _Rect.top;

        for (const FrequencyBand & Iter : frequencyBands)
        {
            D2D1_RECT_F Rect = { x1, y1, x2 - PaddingX, y2 - PaddingY };

            // Draw the background.
            if (_DrawBandBackground)
                renderTarget->FillRectangle(Rect, _BackgroundBrush);

            // Don't render anything above the Nyquist frequency.
            if (Iter.Ctr < (sampleRate / 2.))
            {
                // Draw the foreground.
                if (Iter.CurValue > 0.0)
                {
                    Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * configuration.ScaleA(Iter.CurValue))), y1, Rect.bottom);

                    renderTarget->FillRectangle(Rect, _ForegroundBrush);
                }

                // Draw the peak indicator.
                if ((configuration._PeakMode != PeakMode::None) && (Iter.Peak > 0.))
                {
                    Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * Iter.Peak)), y1, Rect.bottom);
                    Rect.bottom = Rect.top + 1.f;

                    ID2D1Brush * Brush = (configuration._PeakMode != PeakMode::FadeOut) ? (ID2D1Brush *) _ForegroundBrush : (ID2D1Brush *) _WhiteBrush;

                    if (configuration._PeakMode == PeakMode::FadeOut)
                        Brush->SetOpacity((FLOAT) Iter.Opacity);

                    renderTarget->FillRectangle(Rect, Brush);

                    if (configuration._PeakMode == PeakMode::FadeOut)
                        Brush->SetOpacity(1.f);
                }
            }

            x1  = x2;
            x2 += BandWidth;
        }

        return S_OK;
    }

    /// <summary>
    /// Creates resources which are bound to a particular D3D device.
    /// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
    /// </summary>
    HRESULT CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
    {
        HRESULT hr = S_OK;

        if ((_ForegroundBrush == nullptr) && SUCCEEDED(hr))
        {
            if (!_GradientStops.empty())
            {
                CComPtr<ID2D1GradientStopCollection> Collection;

                hr = renderTarget->CreateGradientStopCollection(&_GradientStops[0], (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

                if (SUCCEEDED(hr))
                {
                    D2D1_SIZE_F Size = renderTarget->GetSize();

                    hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_ForegroundBrush);
                }
            }
        }

        if ((_BackgroundBrush == nullptr) && SUCCEEDED(hr))
            hr = renderTarget->CreateSolidColorBrush(_Configuration->_BandBackColor, &_BackgroundBrush);

        if ((_WhiteBrush == nullptr) && SUCCEEDED(hr))
            hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_WhiteBrush);

        return hr;
    }

    /// <summary>
    /// Releases the device specific resources.
    /// </summary>
    void ReleaseDeviceSpecificResources()
    {
        _WhiteBrush.Release();

        _BackgroundBrush.Release();
        _ForegroundBrush.Release();
    }

private:
    const Configuration * _Configuration;

    const FLOAT PaddingX = 1.f;
    const FLOAT PaddingY = 1.f;

    D2D1_RECT_F _Rect;

    bool _DrawBandBackground;
    std::vector<D2D1_GRADIENT_STOP> _GradientStops;

    CComPtr<ID2D1SolidColorBrush> _WhiteBrush;

    CComPtr<ID2D1SolidColorBrush> _BackgroundBrush;
    CComPtr<ID2D1LinearGradientBrush> _ForegroundBrush;
};
