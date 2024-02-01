
/** $VER: Spectrum.cpp (2024.02.01) P. Stuer **/

#include "Spectrum.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#include "BezierSpline.h"

#include "StyleManager.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void Spectrum::Initialize(const Configuration * configuration)
{
    _Configuration = configuration;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void Spectrum::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void Spectrum::Render(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    CreateDeviceSpecificResources(renderTarget);

    switch (_Configuration->_VisualizationType)
    {
        default:

        case VisualizationType::Bars:
            RenderBars(renderTarget, frequencyBands, sampleRate);
            break;

        case VisualizationType::Curve:
            RenderCurve(renderTarget, frequencyBands, sampleRate);
            break;
    }
}

/// <summary>
/// Renders the spectrum analysis as bars.
/// </summary>
void Spectrum::RenderBars(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT BandWidth = ::ceil(Max((Width / (FLOAT) frequencyBands.size()), 1.f));

    FLOAT x1 = _Bounds.left;
    FLOAT x2 = x1 + BandWidth;

    Style & ForegroundStyle = _StyleManager.GetStyle(VisualElement::BarForeground);
    Style & DarkBackgroundStyle = _StyleManager.GetStyle(VisualElement::BarDarkBackground);
    Style & LightBackgroundStyle = _StyleManager.GetStyle(VisualElement::BarLightBackground);
    Style & PeakIndicatorStyle = _StyleManager.GetStyle(VisualElement::BarPeakIndicator);

    ID2D1Brush * Foreground = _Configuration->_HorizontalGradient ? _SolidColorBrush : ForegroundStyle._Brush;

    for (const FrequencyBand & Iter : frequencyBands)
    {
        D2D1_RECT_F Rect = { x1, _Bounds.top, x2 - PaddingX, Height - PaddingY };

        // Draw the bar background, even above the Nyquist frequency.
        if (_Configuration->_DrawBandBackground)
            renderTarget->FillRectangle(Rect, Iter.HasDarkBackground ? DarkBackgroundStyle._Brush : LightBackgroundStyle._Brush);

        // Don't render bar foreground above the Nyquist frequency.
        if (Iter.Ctr < (sampleRate / 2.))
        {
            // Draw the foreground.
            if (Iter.CurValue > 0.0)
            {
                Rect.top = Clamp((FLOAT)(_Bounds.bottom - (Height * _Configuration->ScaleA(Iter.CurValue))), _Bounds.top, _Bounds.bottom);

                if (_Configuration->_HorizontalGradient)
                {
                    _SolidColorBrush->SetColor(Iter.GradientColor);
                    renderTarget->FillRectangle(Rect, _SolidColorBrush);
                }
                else
                    renderTarget->FillRectangle(Rect, Foreground);

                if (_Configuration->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PatternBrush);
            }

            // Draw the peak indicator.
            if ((_Configuration->_PeakMode != PeakMode::None) && (Iter.Peak > 0.))
            {
                Rect.top    = ::ceil(Clamp((FLOAT)(_Bounds.bottom - (Height * Iter.Peak)), _Bounds.top, _Bounds.bottom));
                Rect.bottom = ::ceil(Rect.top + 1.f);

                FLOAT Opacity = ((_Configuration->_PeakMode == PeakMode::FadeOut) || (_Configuration->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) Iter.Opacity : PeakIndicatorStyle._Opacity;

                PeakIndicatorStyle._Brush->SetOpacity(Opacity);

                renderTarget->FillRectangle(Rect, PeakIndicatorStyle._Brush);
            }
        }

        x1  = x2;
        x2 += BandWidth;
    }
}

/// <summary>
/// Renders the spectrum analysis as a curve.
/// </summary>
void Spectrum::RenderCurve(ID2D1RenderTarget * renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    HRESULT hr = S_OK;

    GeometryPoints gp;
    CComPtr<ID2D1PathGeometry> Curve;

    if (_Configuration->_PeakMode != PeakMode::None)
    {
        gp.Clear();

        hr = CreateGeometryPointsFromAmplitude(frequencyBands, sampleRate, true, gp);

        // Draw the area with the peak values.
        if (SUCCEEDED(hr))
        {
            hr = CreateCurve(gp, true, &Curve);

            if (SUCCEEDED(hr))
            {
                Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakArea);

                renderTarget->FillGeometry(Curve, style._Brush);
            }

            Curve.Release();
        }

        // Draw the line with the peak values.
        if (SUCCEEDED(hr))
        {
            hr = CreateCurve(gp, false, &Curve);

            if (SUCCEEDED(hr))
            {
                Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakLine);

                renderTarget->DrawGeometry(Curve, style._Brush, style._Thickness);
            }

            Curve.Release();
        }
    }

    gp.Clear();

    hr = CreateGeometryPointsFromAmplitude(frequencyBands, sampleRate, false, gp);

    // Draw the area with the current values.
    if (SUCCEEDED(hr))
    {
        hr = CreateCurve(gp, true, &Curve);

        if (SUCCEEDED(hr))
        {
            Style & style = _StyleManager.GetStyle(VisualElement::CurveArea);

            renderTarget->FillGeometry(Curve, style._Brush);
        }

        Curve.Release();
    }

    // Draw the line with the current values.
    if (SUCCEEDED(hr))
    {
        hr = CreateCurve(gp, false, &Curve);

        if (SUCCEEDED(hr))
        {
            Style & style = _StyleManager.GetStyle(VisualElement::CurveLine);

            renderTarget->DrawGeometry(Curve, style._Brush, style._Thickness);
        }

        Curve.Release();
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Spectrum::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if ((_SolidColorBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_SolidColorBrush);

    if ((_PatternBrush == nullptr) && SUCCEEDED(hr))
        hr = CreatePatternBrush(renderTarget);

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarForeground);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarDarkBackground);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarLightBackground);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarPeakIndicator);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveLine);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveArea);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakLine);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakArea);

        if (SUCCEEDED(hr) && (style._Brush == nullptr))
        {
            if (style._ColorSource != ColorSource::Gradient)
                hr = renderTarget->CreateSolidColorBrush(style._Color, (ID2D1SolidColorBrush ** ) &style._Brush);
            else
            {
                ID2D1LinearGradientBrush * Brush;

                hr = CreateGradientBrush(renderTarget, style._GradientStops, &Brush);

                if (SUCCEEDED(hr))
                    style._Brush.Attach(Brush);
            }

            style._Brush->SetOpacity(style._Opacity);
        }
    }

    return hr;
}

/// <summary>
/// Creates a gradient brush for rendering the bars.
/// </summary>
HRESULT Spectrum::CreateGradientBrush(ID2D1RenderTarget * renderTarget, const GradientStops & gradientStops, ID2D1LinearGradientBrush ** gradientBrush)
{
    if (gradientStops.empty())
        return E_FAIL;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = renderTarget->CreateGradientStopCollection(&gradientStops[0], (UINT32) gradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F Size = renderTarget->GetSize();

        D2D1_POINT_2F Start = _Configuration->_HorizontalGradient ? D2D1::Point2F(       0.f, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, 0.f);
        D2D1_POINT_2F End   = _Configuration->_HorizontalGradient ? D2D1::Point2F(Size.width, Size.height / 2.f) : D2D1::Point2F(Size.width / 2.f, Size.height);

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(Start, End), Collection, gradientBrush);
    }

    return hr;
}

/// <summary>
/// Creates a pattern brush for rendering LED mode.
/// </summary>
HRESULT Spectrum::CreatePatternBrush(ID2D1RenderTarget * renderTarget)
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(8.f, 4.f), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.f, 0.f, 0.f, 1.f)), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->FillRectangle(D2D1::RectF(0.f, 2.f, 8.f, 4.f), Brush);

            rt->EndDraw();
        }

        if (SUCCEEDED(hr))
        {
            CComPtr<ID2D1Bitmap> Bitmap;

            hr = rt->GetBitmap(&Bitmap);

            if (SUCCEEDED(hr))
            {
                auto brushProperties = D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

                hr = rt->CreateBitmapBrush(Bitmap, brushProperties, &_PatternBrush);
            }
        }
    }

    return hr;
}

/// <summary>
/// Creates the geometry points from the amplitudes of the spectrum.
/// </summary>
HRESULT Spectrum::CreateGeometryPointsFromAmplitude(const std::vector<FrequencyBand> & frequencyBands, double sampleRate, bool usePeak, GeometryPoints & gp)
{
    if (frequencyBands.size() < 2)
        return E_FAIL;

    bool IsFlatLine = true;

    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

    FLOAT x = _Bounds.left + (BandWidth / 2.f); // Make sure the knots are nicely centered in the bar rectangle.
    FLOAT y = 0.f;

    // Create all the knots.
    for (size_t i = 0; i < frequencyBands.size(); ++i)
    {
        // Don't render anything above the Nyquist frequency.
        if (frequencyBands[i].Ctr > (sampleRate / 2.))
            break;

        double Value = !usePeak ? _Configuration->ScaleA(frequencyBands[i].CurValue) : frequencyBands[i].Peak;

        y = Clamp((FLOAT)(_Bounds.bottom - (Height * Value)), _Bounds.top, _Bounds.bottom);

        gp.p0.push_back(D2D1::Point2F(x, y));

        if (y < _Bounds.bottom)
            IsFlatLine = false;

        x += BandWidth;
    }

    const size_t n = gp.p0.size();

    if (n > 1)
    {
        gp.p1.reserve(n - 1);
        gp.p2.reserve(n - 1);

        // Create all the control points.
        BezierSpline::GetControlPoints(gp.p0, n, gp.p1, gp.p2);

        for (size_t i = 0; i < (n - 1); ++i)
        {
            gp.p1[i].y = Clamp(gp.p1[i].y, _Bounds.top, _Bounds.bottom);
            gp.p2[i].y = Clamp(gp.p2[i].y, _Bounds.top, _Bounds.bottom);
        }
    }

    return !IsFlatLine ? S_OK : E_FAIL;
}

/// <summary>
/// Creates a curve from the power values.
/// </summary>
HRESULT Spectrum::CreateCurve(const GeometryPoints & gp, bool isFilled, ID2D1PathGeometry ** curve) const noexcept
{
    if (gp.p0.size() < 2)
        return E_FAIL;

    HRESULT hr = _Direct2D.Factory->CreatePathGeometry(curve);

    CComPtr<ID2D1GeometrySink> Sink;

    if (SUCCEEDED(hr))
        hr = (*curve)->Open(&Sink);

    if (SUCCEEDED(hr))
    {
        Sink->SetFillMode(D2D1_FILL_MODE_WINDING);

        if (isFilled)
        {
            Sink->BeginFigure(D2D1::Point2F(_Bounds.left, _Bounds.bottom), D2D1_FIGURE_BEGIN_FILLED); // Start with a vertical line going up.
            Sink->AddLine(D2D1::Point2F(_Bounds.left, gp.p0[0].y));
        }
        else
            Sink->BeginFigure(D2D1::Point2F(_Bounds.left, gp.p0[0].y), D2D1_FIGURE_BEGIN_HOLLOW);

        const size_t n = gp.p1.size();

        for (size_t i = 0; i < n; ++i)
            Sink->AddBezier(D2D1::BezierSegment(gp.p1[i], gp.p2[i], gp.p0[i + 1]));

        if (isFilled)
            Sink->AddLine(D2D1::Point2F(gp.p0[n].x, _Bounds.bottom)); // End with a vertical line going down.

        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

        hr = Sink->Close();
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void Spectrum::ReleaseDeviceSpecificResources()
{
    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarForeground);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarDarkBackground);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarLightBackground);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::BarPeakIndicator);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakArea);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurvePeakLine);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveArea);

        style._Brush.Release();
    }

    {
        Style & style = _StyleManager.GetStyle(VisualElement::CurveLine);

        style._Brush.Release();
    }

    _PatternBrush.Release();

    _SolidColorBrush.Release();
}
