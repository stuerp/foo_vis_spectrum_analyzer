
/** $VER: Spectrum.cpp (2023.12.11) P. Stuer **/

#include "Spectrum.h"

#pragma hdrstop

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
HRESULT Spectrum::Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate, const Configuration & configuration)
{
    CreateDeviceSpecificResources(renderTarget);

    const FLOAT Width = _Rect.right - _Rect.left;
    const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

    FLOAT x1 = _Rect.left;
    FLOAT x2 = x1 + BandWidth;

    const FLOAT y1 = _Rect.top;
    const FLOAT y2 = _Rect.bottom - _Rect.top;

    for (const FrequencyBand & Iter : frequencyBands)
    {
        D2D1_RECT_F Rect = { x1, y1, x2 - PaddingX, y2 - PaddingY };

        // Draw the background.
        if (_DrawBandBackground)
            renderTarget->FillRectangle(Rect, _BandBackgroundBrush);

        // Don't render anything above the Nyquist frequency.
        if (Iter.Ctr < (sampleRate / 2.))
        {
            // Draw the foreground.
            if (Iter.CurValue > 0.0)
            {
                Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * configuration.ScaleA(Iter.CurValue))), y1, Rect.bottom);

                renderTarget->FillRectangle(Rect, _BandForegroundBrush);

                if (_Configuration->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PatternBrush);
            }

            // Draw the peak indicator.
            if ((configuration._PeakMode != PeakMode::None) && (Iter.Peak > 0.))
            {
                Rect.top = Clamp((FLOAT)(y2 - ((y2 - y1) * Iter.Peak)), y1, Rect.bottom);
                Rect.bottom = Rect.top + 1.f;

                ID2D1Brush * Brush = (configuration._PeakMode != PeakMode::FadeOut) ? (ID2D1Brush *) _BandForegroundBrush : (ID2D1Brush *) _WhiteBrush;

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
HRESULT Spectrum::CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    HRESULT hr = S_OK;

    if ((_BandForegroundBrush == nullptr) && SUCCEEDED(hr))
    {
        if (!_GradientStops.empty())
        {
            CComPtr<ID2D1GradientStopCollection> Collection;

            hr = renderTarget->CreateGradientStopCollection(&_GradientStops[0], (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

            if (SUCCEEDED(hr))
            {
                D2D1_SIZE_F Size = renderTarget->GetSize();

                hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_BandForegroundBrush);
            }
        }
    }

    if ((_BandBackgroundBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(_Configuration->_BandBackColor, &_BandBackgroundBrush);

    if ((_WhiteBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_WhiteBrush);

    if ((_PatternBrush == nullptr) && SUCCEEDED(hr))
        hr = CreatePatternBrush(renderTarget);

    return hr;
}

/// <summary>
/// Creates a pattern brush for rendering LED mode.
/// </summary>
HRESULT Spectrum::CreatePatternBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    ID2D1BitmapRenderTarget * rt = nullptr;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(8.f, 4.f), &rt);

    if (SUCCEEDED(hr))
    {
        ID2D1SolidColorBrush * Brush = nullptr;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.f, 0.f, 0.f, 1.f)), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->FillRectangle(D2D1::RectF(0.f, 2.f, 8.f, 4.f), Brush);

            rt->EndDraw();

            Brush->Release();
        }

        if (SUCCEEDED(hr))
        {
            ID2D1Bitmap * Bitmap = nullptr;

            hr = rt->GetBitmap(&Bitmap);

            if (SUCCEEDED(hr))
            {
                auto brushProperties = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

                hr = rt->CreateBitmapBrush(Bitmap, brushProperties, &_PatternBrush);

                Bitmap->Release();
            }
        }

        rt->Release();
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectrum::ReleaseDeviceSpecificResources()
{
    _PatternBrush.Release();

    _WhiteBrush.Release();

    _BandBackgroundBrush.Release();
    _BandForegroundBrush.Release();
}
