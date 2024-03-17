
/** $VER: Spectrum.cpp (2024.03.17) P. Stuer **/

#include "Spectrum.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#include "BezierSpline.h"

#include "StyleManager.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void Spectrum::Initialize(State * state, const GraphSettings * settings)
{
    _State = state;
    _GraphSettings = settings;

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
void Spectrum::Render(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    D2D1::Matrix3x2F Transform = D2D1::Matrix3x2F::Identity();

    if (_GraphSettings->_FlipHorizontally)
    {
        const FLOAT Width = _Bounds.right - _Bounds.left;

        Transform = D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, Width, 0.f);
    }

    if (!_GraphSettings->_FlipVertically) // Negate because the GUI assumes the mathematical (bottom-left 0,0) coordinate system.
    {
        const FLOAT Height = _Bounds.bottom - _Bounds.top;

        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, Height);

        Transform = Transform * FlipV;
    }

    D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Bounds.left, _Bounds.top);

    renderTarget->SetTransform(Transform * Translate);

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
    {
        switch (_State->_VisualizationType)
        {
            default:

            case VisualizationType::Bars:
                RenderBars(renderTarget, frequencyBands, sampleRate);
                break;

            case VisualizationType::Curve:
                RenderCurve(renderTarget, frequencyBands, sampleRate);
                break;

            case VisualizationType::Spectogram:
                break;
        }

        if (_NyquistMarker->_ColorSource != ColorSource::None)
            RenderNyquistFrequencyMarker(renderTarget, frequencyBands, sampleRate);
    }

    renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Renders the spectrum analysis as bars.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectrum::RenderBars(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT Bandwidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

    const FLOAT PeakThickness = _PeakTop->_Thickness / 2.f;
    const FLOAT BarThickness = _BarTop->_Thickness / 2.f;

    FLOAT x1 = 0.f;
    FLOAT x2 = x1 + Bandwidth;

    auto OldAntialiasMode = renderTarget->GetAntialiasMode();

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    for (const auto & fb : frequencyBands)
    {
        assert(InRange(fb.CurValue, 0.0, 1.0));
        assert(InRange(fb.Peak, 0.0, 1.0));

        D2D1_RECT_F Rect = { x1, 0.f, x2 - PaddingX, Height - PaddingY };

        // Draw the bar background, even above the Nyquist frequency.
        if (fb.HasDarkBackground)
        {
            if (_DarkBackground->_ColorSource != ColorSource::None)
                renderTarget->FillRectangle(Rect, _DarkBackground->_Brush);
        }
        else
        {
            if (_LightBackground->_ColorSource != ColorSource::None)
                renderTarget->FillRectangle(Rect, _LightBackground->_Brush);
        }

        const bool GreaterThanNyquist = fb.Ctr >= (sampleRate / 2.);

        if (!GreaterThanNyquist || (GreaterThanNyquist && !_State->_SuppressMirrorImage))
        {
            if ((_State->_PeakMode != PeakMode::None) && (fb.Peak > 0.))
            {
                Rect.top    = 0.f;
                Rect.bottom = (FLOAT) (Height * fb.Peak);

                // Draw the peak indicator area.
                if (_PeakArea->_ColorSource != ColorSource::None)
                {
                    if ((_PeakArea->_Flags & (Style::HorizontalGradient | Style::AmplitudeBasedColor)) == (Style::HorizontalGradient | Style::AmplitudeBasedColor))
                        _PeakArea->SetBrushColor(fb.Peak);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakArea->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakArea->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the peak indicator top.
                if (_PeakTop->_ColorSource != ColorSource::None)
                {
                    Rect.top    = ::ceil(Clamp(Rect.bottom - PeakThickness, 0.f, Height));
                    Rect.bottom = ::ceil(Clamp(Rect.top    + PeakThickness, 0.f, Height));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) fb.Opacity : _PeakTop->_Opacity;

                    _PeakTop->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _PeakTop->_Brush);
                }
            }

            {
                Rect.top    = 0.f;
                Rect.bottom = (FLOAT) (Height * fb.CurValue);

                // Draw the area of the bar.
                if (_BarArea->_ColorSource != ColorSource::None)
                {
                    if ((_BarArea->_Flags & (Style::HorizontalGradient | Style::AmplitudeBasedColor)) == (Style::HorizontalGradient | Style::AmplitudeBasedColor))
                        _BarArea->SetBrushColor(fb.CurValue);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _BarArea->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _BarArea->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the top of the bar.
                if (_BarTop->_ColorSource != ColorSource::None)
                {
                    Rect.top    = Clamp(Rect.bottom - BarThickness, 0.f, Height);
                    Rect.bottom = Clamp(Rect.top    + BarThickness, 0.f, Height);

                    renderTarget->FillRectangle(Rect, _BarTop->_Brush);
                }
            }
        }

        x1 = ::round(x2);
        x2 = x1 + Bandwidth;
    }

    if (_State->_LEDMode)
        renderTarget->SetAntialiasMode(OldAntialiasMode);
}

/// <summary>
/// Renders the spectrum analysis as a curve.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectrum::RenderCurve(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate)
{
    HRESULT hr = S_OK;

    GeometryPoints Points;
    CComPtr<ID2D1PathGeometry> Curve;

    if ((_State->_PeakMode != PeakMode::None) && (_CurvePeakArea->_ColorSource != ColorSource::None || _CurvePeakLine->_ColorSource != ColorSource::None))
    {
        Points.Clear();

        hr = CreateGeometryPointsFromAmplitude(frequencyBands, sampleRate, true, Points);

        // Draw the area with the peak values.
        if (SUCCEEDED(hr) && (_CurvePeakArea->_ColorSource != ColorSource::None))
        {
            hr = CreateCurve(Points, true, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->FillGeometry(Curve, _CurvePeakArea->_Brush);

            Curve.Release();
        }

        // Draw the line with the peak values.
        if (SUCCEEDED(hr) && (_CurvePeakLine->_ColorSource != ColorSource::None))
        {
            hr = CreateCurve(Points, false, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->DrawGeometry(Curve, _CurvePeakLine->_Brush, _CurvePeakLine->_Thickness);

            Curve.Release();
        }
    }

    if ((_CurveArea->_ColorSource != ColorSource::None || _CurveLine->_ColorSource != ColorSource::None))
    {
        Points.Clear();

        hr = CreateGeometryPointsFromAmplitude(frequencyBands, sampleRate, false, Points);

        // Draw the area with the current values.
        if (SUCCEEDED(hr) && (_CurveArea->_ColorSource != ColorSource::None))
        {
            hr = CreateCurve(Points, true, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->FillGeometry(Curve, _CurveArea->_Brush);

            Curve.Release();
        }

        // Draw the line with the current values.
        if (SUCCEEDED(hr) && (_CurveLine->_ColorSource != ColorSource::None))
        {
            hr = CreateCurve(Points, false, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->DrawGeometry(Curve, _CurveLine->_Brush, _CurveLine->_Thickness);

            Curve.Release();
        }
    }
}

/// <summary>
/// Renders a marker for the Nyquist frequency.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void Spectrum::RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget, const FrequencyBands & frequencyBands, double sampleRate) const noexcept
{
    const double MinScale = ScaleF(frequencyBands.front().Ctr, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(frequencyBands.back() .Ctr, _State->_ScalingFunction, _State->_SkewFactor);

    const double NyquistScale = Clamp(ScaleF(sampleRate / 2., _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale);

    FLOAT x = Map(NyquistScale, MinScale, MaxScale, 0.f, _Bounds.right - _Bounds.left);

    renderTarget->DrawLine(D2D1_POINT_2F(x, 0.f), D2D1_POINT_2F(x, _Bounds.bottom - _Bounds.top), _NyquistMarker->_Brush, _NyquistMarker->_Thickness, nullptr);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Spectrum::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(renderTarget);

    const D2D1_SIZE_F Size = { _Bounds.right - _Bounds.left, _Bounds.bottom - _Bounds.top };

    if (SUCCEEDED(hr))
    {
        if (_BarArea == nullptr)
            _BarArea = _State->_StyleManager.GetStyle(VisualElement::BarArea);

        if (_BarArea && (_BarArea->_Brush == nullptr))
            hr = _BarArea->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_BarTop == nullptr)
            _BarTop = _State->_StyleManager.GetStyle(VisualElement::BarTop);

        if (_BarTop && (_BarTop->_Brush == nullptr))
            hr = _BarTop->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_PeakArea == nullptr)
            _PeakArea = _State->_StyleManager.GetStyle(VisualElement::BarPeakArea);

        if (_PeakArea && (_PeakArea->_Brush == nullptr))
            hr = _PeakArea->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_PeakTop == nullptr)
            _PeakTop = _State->_StyleManager.GetStyle(VisualElement::BarPeakTop);

        if (_PeakTop && (_PeakTop->_Brush == nullptr))
            hr = _PeakTop->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_DarkBackground == nullptr)
            _DarkBackground = _State->_StyleManager.GetStyle(VisualElement::BarDarkBackground);

        if (_DarkBackground && (_DarkBackground->_Brush == nullptr))
            hr = _DarkBackground->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_LightBackground == nullptr)
            _LightBackground = _State->_StyleManager.GetStyle(VisualElement::BarLightBackground);

        if (_LightBackground && (_LightBackground->_Brush == nullptr))
            hr = _LightBackground->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_CurveLine == nullptr)
            _CurveLine = _State->_StyleManager.GetStyle(VisualElement::CurveLine);

        if (_CurveLine && (_CurveLine->_Brush == nullptr))
            hr = _CurveLine->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_CurveArea == nullptr)
            _CurveArea = _State->_StyleManager.GetStyle(VisualElement::CurveArea);

        if (_CurveArea && (_CurveArea->_Brush == nullptr))
            hr = _CurveArea->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_CurvePeakLine == nullptr)
            _CurvePeakLine = _State->_StyleManager.GetStyle(VisualElement::CurvePeakLine);

        if (_CurvePeakLine && (_CurvePeakLine->_Brush == nullptr))
            hr = _CurvePeakLine->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_CurvePeakArea == nullptr)
            _CurvePeakArea = _State->_StyleManager.GetStyle(VisualElement::CurvePeakArea);

        if (_CurvePeakArea && (_CurvePeakArea->_Brush == nullptr))
            hr = _CurvePeakArea->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
        if (_NyquistMarker == nullptr)
            _NyquistMarker = _State->_StyleManager.GetStyle(VisualElement::NyquistMarker);

        if (_NyquistMarker && (_NyquistMarker->_Brush == nullptr))
            hr = _NyquistMarker->CreateDeviceSpecificResources(renderTarget, Size);
    }

    if (SUCCEEDED(hr))
    {
    }

    return hr;
}

/// <summary>
/// Creates an opacity mask to render the LED's.
/// </summary>
HRESULT Spectrum::CreateOpacityMask(ID2D1RenderTarget * renderTarget)
{
    D2D1_SIZE_F Size = renderTarget->GetSize();

    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(1.f, Size.height), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(0.f, 0.f, 0.f, 1.f)), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            for (FLOAT y = 2.f; y < Size.height; y += 4)
                rt->FillRectangle(D2D1::RectF(0.f, y, 1.f, y + 2.f), Brush);

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}

/// <summary>
/// Creates the geometry points from the amplitudes of the spectrum.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
HRESULT Spectrum::CreateGeometryPointsFromAmplitude(const FrequencyBands & frequencyBands, double sampleRate, bool usePeak, GeometryPoints & points)
{
    if (frequencyBands.size() < 2)
        return E_FAIL;

    bool IsFlatLine = true;

    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

    FLOAT x = BandWidth / 2.f; // Make sure the knots are nicely centered in the band rectangle.
    FLOAT y = 0.f;

    // Create all the knots.
    for (const auto & fb: frequencyBands)
    {
        // Don't render anything above the Nyquist frequency.
        if ((fb.Ctr > (sampleRate / 2.)) && _State->_SuppressMirrorImage)
            break;

//      double Value = !usePeak ? _GraphSettings->ScaleA(fb.CurValue) : fb.Peak;
        double Value = !usePeak ? fb.CurValue : fb.Peak;

        y = Clamp((FLOAT)(Value * Height), 0.f, Height);

        points.p0.push_back(D2D1::Point2F(x, y));

        if (y > 0.f)
            IsFlatLine = false;

        x += BandWidth;
    }

    // Create all the control points.
    const size_t n = points.p0.size();

    if (n > 1)
    {
        points.p1.reserve(n - 1);
        points.p2.reserve(n - 1);

        // Create all the control points.
        BezierSpline::GetControlPoints(points.p0, n, points.p1, points.p2);

        for (size_t i = 0; i < (n - 1); ++i)
        {
            points.p1[i].y = Clamp(points.p1[i].y, 0.f, Height);
            points.p2[i].y = Clamp(points.p2[i].y, 0.f, Height);
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
            Sink->BeginFigure(D2D1::Point2F(0.f, 0.f), D2D1_FIGURE_BEGIN_FILLED); // Start with a vertical line going up.
            Sink->AddLine(D2D1::Point2F(0.f, gp.p0[0].y));
        }
        else
            Sink->BeginFigure(D2D1::Point2F(0.f, gp.p0[0].y), D2D1_FIGURE_BEGIN_HOLLOW);

        const size_t n = gp.p1.size();

        for (size_t i = 0; i < n; ++i)
            Sink->AddBezier(D2D1::BezierSegment(gp.p1[i], gp.p2[i], gp.p0[i + 1]));

        if (isFilled)
            Sink->AddLine(D2D1::Point2F(gp.p0[n].x, 0.f)); // End with a vertical line going down.

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
    _CurveLine = nullptr;
    _CurveArea = nullptr;
    _CurvePeakLine = nullptr;
    _CurvePeakArea = nullptr;

    _BarArea = nullptr;
    _BarTop = nullptr;
    _PeakArea = nullptr;
    _PeakTop = nullptr;
    _DarkBackground = nullptr;
    _LightBackground = nullptr;

    _NyquistMarker = nullptr;

    for (const auto & Iter :
    {
        VisualElement::BarArea, VisualElement::BarTop, VisualElement::BarPeakArea, VisualElement::BarPeakTop, VisualElement::BarDarkBackground, VisualElement::BarLightBackground,
        VisualElement::CurveLine, VisualElement::CurveArea, VisualElement::CurvePeakLine, VisualElement::CurvePeakArea,
        VisualElement::NyquistMarker
    })
        _State->_StyleManager.GetStyle(Iter)->ReleaseDeviceSpecificResources();

    _OpacityMask.Release();
}
