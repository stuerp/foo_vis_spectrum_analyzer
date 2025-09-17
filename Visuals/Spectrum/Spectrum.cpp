
/** $VER: Spectrum.cpp (2025.09.17) P. Stuer **/

#include "pch.h"
#include "Spectrum.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#include "BezierSpline.h"

#include "StyleManager.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void spectrum_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();

    _XAxis.Initialize(state, settings, analysis);
    _YAxis.Initialize(state, settings, analysis);

    _Chrono.Reset();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void spectrum_t::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

    _OpacityMask.Release();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void spectrum_t::Resize() noexcept
{
    if (!_IsResized ||(_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    const FLOAT xt = ((_GraphSettings->_XAxisMode != XAxisMode::None) && _GraphSettings->_XAxisTop)    ? _XAxis.GetTextHeight() : 0.f;
    const FLOAT xb = ((_GraphSettings->_XAxisMode != XAxisMode::None) && _GraphSettings->_XAxisBottom) ? _XAxis.GetTextHeight() : 0.f;

    const FLOAT yl = ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisLeft)   ? _YAxis.GetTextWidth()  : 0.f;
    const FLOAT yr = ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisRight)  ? _YAxis.GetTextWidth()  : 0.f;

    _XAxis.Move({ _Bounds.left + yl, _Bounds.top,      _Bounds.right - yr, _Bounds.bottom });
    _YAxis.Move({ _Bounds.left,      _Bounds.top + xt, _Bounds.right,      _Bounds.bottom - xb });

    _ClientBounds = { _Bounds.left + yl, _Bounds.top + xt, _Bounds.right - yr, _Bounds.bottom - xb };
    _ClientSize = { _ClientBounds.right - _ClientBounds.left, _ClientBounds.bottom - _ClientBounds.top };

    _IsResized = false;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void spectrum_t::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if (_State->_VisualizationType != VisualizationType::RadialBars)
    {
        _XAxis.Render(renderTarget);

        _YAxis.Render(renderTarget);

        {
            SetTransform(renderTarget, _ClientBounds);

            if (_State->_VisualizationType == VisualizationType::Bars)
                RenderBars(renderTarget);
            else
            if (_State->_VisualizationType == VisualizationType::Curve)
                RenderCurve(renderTarget);
  
            if (_NyquistMarkerStyle->IsEnabled())
                RenderNyquistFrequencyMarker(renderTarget);

            ResetTransform(renderTarget);
        }
    }
    else
    {
        const D2D1::Matrix3x2F FlipV = D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, _Size.height);
        const D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(_Size.width / 2.f, _Size.height / 2.f);

        renderTarget->SetTransform(Translate * FlipV);

        RenderRadialBars(renderTarget);

        ResetTransform(renderTarget);
    }
}

/// <summary>
/// Renders the spectrum analysis as bars.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void spectrum_t::RenderBars(ID2D1RenderTarget * renderTarget)
{
    const FLOAT Bandwidth = std::max(::floor(_ClientSize.width / (FLOAT) _Analysis->_FrequencyBands.size()), 2.f); // In DIP
    const FLOAT SpectrumWidth = Bandwidth * (FLOAT) _Analysis->_FrequencyBands.size();

    const FLOAT PeakThickness = _PeakTopStyle->_Thickness / 2.f;
    const FLOAT BarThickness = _BarTopStyle->_Thickness / 2.f;

    const FLOAT HOffset = (_GraphSettings->_HorizontalAlignment == HorizontalAlignment::Near) ? 0.f : ((_GraphSettings->_HorizontalAlignment == HorizontalAlignment::Center) ? (_ClientSize.width - SpectrumWidth) / 2.f : (_ClientSize.width - SpectrumWidth));

    FLOAT x1 = HOffset;
    FLOAT x2 = x1 + Bandwidth;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    for (const auto & fb : _Analysis->_FrequencyBands)
    {
        assert(msc::InRange(fb.CurValue, 0.0, 1.0));
        assert(msc::InRange(fb.MaxValue, 0.0, 1.0));

        x1 = std::clamp(x1, 0.f, _ClientSize.width);
        x2 = std::clamp(x2, 0.f, _ClientSize.width);

        D2D1_RECT_F Rect = { x1, 0.f, x2 - PaddingX, _ClientSize.height - PaddingY };

        // Draw the bar background, even above the Nyquist frequency.
        if (fb.HasDarkBackground)
        {
            if (_DarkBackgroundStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _DarkBackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _DarkBackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }
        }
        else
        {
            if (_LightBackgroundStyle->IsEnabled())
            {
                if (!_State->_LEDMode)
                    renderTarget->FillRectangle(Rect, _LightBackgroundStyle->_Brush);
                else
                    renderTarget->FillOpacityMask(_OpacityMask, _LightBackgroundStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
            }
        }

        const bool GreaterThanNyquist = fb.Ctr >= _Analysis->_NyquistFrequency;

        if (!GreaterThanNyquist || (GreaterThanNyquist && !_State->_SuppressMirrorImage))
        {
            if ((_State->_PeakMode != PeakMode::None) && (fb.MaxValue > 0.))
            {
                Rect.top    = 0.f;
                Rect.bottom = (FLOAT) (_ClientSize.height * fb.MaxValue);

                // Draw the peak indicator area.
                if (_PeakAreaStyle->IsEnabled())
                {
                    if (_PeakAreaStyle->IsAmplitudeBased())
                        _PeakAreaStyle->SetBrushColor(fb.MaxValue);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _PeakAreaStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _PeakAreaStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the peak indicator top.
                if (_PeakTopStyle->IsEnabled())
                {
                    Rect.top    = ::ceil(std::clamp(Rect.bottom - PeakThickness, 0.f, _ClientSize.height));
                    Rect.bottom = ::ceil(std::clamp(Rect.top    + PeakThickness, 0.f, _ClientSize.height));

                    FLOAT Opacity = ((_State->_PeakMode == PeakMode::FadeOut) || (_State->_PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) fb.Opacity : _PeakTopStyle->_Opacity;

                    _PeakTopStyle->_Brush->SetOpacity(Opacity);

                    renderTarget->FillRectangle(Rect, _PeakTopStyle->_Brush);
                }
            }

            {
                Rect.top    = 0.f;
                Rect.bottom = (FLOAT) (_ClientSize.height * fb.CurValue);

                // Draw the area of the bar.
                if (_BarAreaStyle->IsEnabled())
                {
                    if (_BarAreaStyle->IsAmplitudeBased())
                        _BarAreaStyle->SetBrushColor(fb.CurValue);

                    if (!_State->_LEDMode)
                        renderTarget->FillRectangle(Rect, _BarAreaStyle->_Brush);
                    else
                        renderTarget->FillOpacityMask(_OpacityMask, _BarAreaStyle->_Brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS, Rect, Rect);
                }

                // Draw the top of the bar.
                if (_BarTopStyle->IsEnabled())
                {
                    Rect.top    = std::clamp(Rect.bottom - BarThickness, 0.f, _ClientSize.height);
                    Rect.bottom = std::clamp(Rect.top    + BarThickness, 0.f, _ClientSize.height);

                    renderTarget->FillRectangle(Rect, _BarTopStyle->_Brush);
                }
            }
        }

        x1 = x2;
        x2 = x1 + Bandwidth;
    }
}

/// <summary>
/// Renders the spectrum analysis as a curve.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void spectrum_t::RenderCurve(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    geometry_points_t Points;
    CComPtr<ID2D1PathGeometry> Curve;

    if ((_State->_PeakMode != PeakMode::None) && (_CurvePeakAreaStyle->IsEnabled() || _CurvePeakLineStyle->IsEnabled()))
    {
        Points.Clear();

        hr = CreateGeometryPointsFromAmplitude(Points, true);

        // Draw the area with the peak values.
        if (SUCCEEDED(hr) && _CurvePeakAreaStyle->IsEnabled())
        {
            hr = CreateCurve(Points, true, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->FillGeometry(Curve, _CurvePeakAreaStyle->_Brush);

            Curve.Release();
        }

        // Draw the line with the peak values.
        if (SUCCEEDED(hr) && _CurvePeakLineStyle->IsEnabled())
        {
            hr = CreateCurve(Points, false, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->DrawGeometry(Curve, _CurvePeakLineStyle->_Brush, _CurvePeakLineStyle->_Thickness);

            Curve.Release();
        }
    }

    if (_CurveAreaStyle->IsEnabled() || _CurveLineStyle->IsEnabled())
    {
        Points.Clear();

        hr = CreateGeometryPointsFromAmplitude(Points, false);

        // Draw the area with the current values.
        if (SUCCEEDED(hr) && _CurveAreaStyle->IsEnabled())
        {
            hr = CreateCurve(Points, true, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->FillGeometry(Curve, _CurveAreaStyle->_Brush);

            Curve.Release();
        }

        // Draw the line with the current values.
        if (SUCCEEDED(hr) && _CurveLineStyle->IsEnabled())
        {
            hr = CreateCurve(Points, false, &Curve);

            if (SUCCEEDED(hr))
                renderTarget->DrawGeometry(Curve, _CurveLineStyle->_Brush, _CurveLineStyle->_Thickness);

            Curve.Release();
        }
    }
}

/// <summary>
/// Renders the spectrum analysis as radial bars.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void spectrum_t::RenderRadialBars(ID2D1RenderTarget * renderTarget)
{
    if (_Analysis->_FrequencyBands.empty())
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    const double Side = std::min(_ClientSize.width / 2.f, _ClientSize.height / 2.f);
    const double InnerRadius =  Side * _State->_InnerRadius;
    const double OuterRadius = (Side * _State->_OuterRadius) - InnerRadius;

    FLOAT a = (FLOAT) ::fmod(M_PI_2 + (_Chrono.Elapsed() * -Degrees2Radians(_State->_AngularVelocity)), 2. * M_PI);
//  FLOAT a = (FLOAT) ::fmod(M_PI_2 + ::cos(_Chrono.Elapsed() * -_State->_AngularVelocity), 2. * M_PI);

    const FLOAT da = (FLOAT)(2. * M_PI) / (FLOAT) _Analysis->_FrequencyBands.size();

    size_t i = 0;
    double n = (double) (_Analysis->_FrequencyBands.size() - 1);

    CComPtr<ID2D1PathGeometry> Path;

    for (const auto & fb : _Analysis->_FrequencyBands)
    {
        HRESULT hr = _Direct2D.Factory->CreatePathGeometry(&Path);

        if (SUCCEEDED(hr))
        {
            CComPtr<ID2D1GeometrySink> Sink;

            hr = Path->Open(&Sink);

            if (SUCCEEDED(hr))
            {
                FLOAT Sin, Cos;

                ::D2D1SinCos(a, &Sin, &Cos);

                FLOAT x = (FLOAT) (Cos * InnerRadius);
                FLOAT y = (FLOAT) (Sin * InnerRadius);

                Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED);

                const double r = InnerRadius + (OuterRadius * fb.CurValue);

                x = (FLOAT) (Cos * r);
                y = (FLOAT) (Sin * r);

                Sink->AddLine(D2D1::Point2F(x, y));

                a -= da;

                ::D2D1SinCos(a, &Sin, &Cos);

                x = (FLOAT) (Cos * r);
                y = (FLOAT) (Sin * r);

                Sink->AddLine(D2D1::Point2F(x, y));

                x = (FLOAT) (Cos * InnerRadius);
                y = (FLOAT) (Sin * InnerRadius);

                Sink->AddLine(D2D1::Point2F(x, y));

                Sink->EndFigure(D2D1_FIGURE_END_CLOSED);

                Sink->Close();

                Sink.Release();

                if (_BarAreaStyle->IsEnabled())
                {
                    if (_BarAreaStyle->Has(style_t::Features::HorizontalGradient))
                    {
                        const double Value = _BarAreaStyle->Has(style_t::Features::AmplitudeBasedColor) ? fb.CurValue : ((double) i / n);

                        _BarAreaStyle->SetBrushColor(Value);
                    }

                    renderTarget->FillGeometry(Path, _BarAreaStyle->_Brush);
                }
            }

            Path.Release();
        }

        ++i;
    }
}

/// <summary>
/// Renders a marker for the Nyquist frequency.
/// Note: Created in a top-left (0,0) coordinate system and later translated and flipped as necessary.
/// </summary>
void spectrum_t::RenderNyquistFrequencyMarker(ID2D1RenderTarget * renderTarget) const noexcept
{
    const double MinScale = ScaleF(_Analysis->_FrequencyBands.front().Ctr, _State->_ScalingFunction, _State->_SkewFactor);
    const double MaxScale = ScaleF(_Analysis->_FrequencyBands.back() .Ctr, _State->_ScalingFunction, _State->_SkewFactor);

    const double NyquistScale = std::clamp(ScaleF(_Analysis->_NyquistFrequency, _State->_ScalingFunction, _State->_SkewFactor), MinScale, MaxScale);

    const FLOAT Bandwidth = std::max(::floor(_ClientSize.width / (FLOAT) _Analysis->_FrequencyBands.size()), 2.f); // In DIP
    const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? Bandwidth * (FLOAT) _Analysis->_FrequencyBands.size() : _ClientSize.width;

    const FLOAT HOffset = (_GraphSettings->_HorizontalAlignment == HorizontalAlignment::Near) ? 0.f : ((_GraphSettings->_HorizontalAlignment == HorizontalAlignment::Center) ? (_ClientSize.width - SpectrumWidth) / 2.f : (_ClientSize.width - SpectrumWidth));

    const FLOAT x1 = HOffset + (Bandwidth / 2.f);

    const FLOAT x = x1 + msc::Map(NyquistScale, MinScale, MaxScale, 0.f, SpectrumWidth);

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    renderTarget->DrawLine(D2D1_POINT_2F(x, 0.f), D2D1_POINT_2F(x, _ClientSize.height), _NyquistMarkerStyle->_Brush, _NyquistMarkerStyle->_Thickness, nullptr);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT spectrum_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _XAxis.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _YAxis.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        Resize();

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(renderTarget);

    if (SUCCEEDED(hr))
    {
        if (_State->_VisualizationType == VisualizationType::Bars)
        {
            hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarArea, renderTarget, _ClientSize, L"", &_BarAreaStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarTop, renderTarget, _ClientSize, L"", &_BarTopStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarPeakArea, renderTarget, _ClientSize, L"", &_PeakAreaStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarPeakTop, renderTarget, _ClientSize, L"", &_PeakTopStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarDarkBackground, renderTarget, _ClientSize, L"", &_DarkBackgroundStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarLightBackground, renderTarget, _ClientSize, L"", &_LightBackgroundStyle);

        }
        else
        if (_State->_VisualizationType == VisualizationType::Curve)
        {
            hr = _State->_StyleManager.GetInitializedStyle(VisualElement::CurveLine, renderTarget, _ClientSize, L"", &_CurveLineStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::CurveArea, renderTarget, _ClientSize, L"", &_CurveAreaStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::CurvePeakLine, renderTarget, _ClientSize, L"", &_CurvePeakLineStyle);

            if (SUCCEEDED(hr))
                hr = _State->_StyleManager.GetInitializedStyle(VisualElement::CurvePeakArea, renderTarget, _ClientSize, L"", &_CurvePeakAreaStyle);

        }
        else
        if (_State->_VisualizationType == VisualizationType::RadialBars)
        {
            hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarArea, renderTarget, _ClientSize, { 0.f, 0.f }, { 0.f, 0.f}, _ClientSize.height / 2.f, _ClientSize.height / 2.f, _State->_InnerRadius, &_BarAreaStyle);
        }
    }

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::NyquistMarker, renderTarget, _ClientSize, L"", &_NyquistMarkerStyle);

    return hr;
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT spectrum_t::CreateOpacityMask(ID2D1RenderTarget * renderTarget)
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = renderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(1.f, _ClientSize.height), &rt);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.f), &Brush);

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->Clear();

            if ((_State->_LEDSize + _State->_LEDGap) > 0.f)
            {
                for (FLOAT y = _State->_LEDGap; y < _ClientSize.height; y += (_State->_LEDSize + _State->_LEDGap))
                    rt->FillRectangle(D2D1::RectF(0.f, y, 1.f, y + _State->_LEDSize), Brush);
            }

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
HRESULT spectrum_t::CreateGeometryPointsFromAmplitude(geometry_points_t & points, bool usePeak) const
{
    if (_Analysis->_FrequencyBands.size() < 2)
        return E_FAIL;

    bool IsFlatLine = true;

    const FLOAT BandWidth = std::max((_ClientSize.width / (FLOAT) _Analysis->_FrequencyBands.size()), 1.f);

    FLOAT x = BandWidth / 2.f; // Make sure the knots are nicely centered in the band rectangle.
    FLOAT y = 0.f;

    // Create all the knots.
    for (const auto & fb: _Analysis->_FrequencyBands)
    {
        // Don't render anything above the Nyquist frequency.
        if ((fb.Ctr > _Analysis->_NyquistFrequency) && _State->_SuppressMirrorImage)
            break;

        double Value = !usePeak ? fb.CurValue : fb.MaxValue;

        y = std::clamp((FLOAT)(Value * _ClientSize.height), 0.f, _ClientSize.height);

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
        bezier_spline_t::GetControlPoints(points.p0, n, points.p1, points.p2);

        for (size_t i = 0; i < (n - 1); ++i)
        {
            points.p1[i].y = std::clamp(points.p1[i].y, 0.f, _ClientSize.height);
            points.p2[i].y = std::clamp(points.p2[i].y, 0.f, _ClientSize.height);
        }
    }

    return !IsFlatLine ? S_OK : E_FAIL;
}

/// <summary>
/// Creates a curve from the power values.
/// </summary>
HRESULT spectrum_t::CreateCurve(const geometry_points_t & gp, bool isFilled, ID2D1PathGeometry ** curve) const noexcept
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
void spectrum_t::ReleaseDeviceSpecificResources()
{
    _YAxis.ReleaseDeviceSpecificResources();
    _XAxis.ReleaseDeviceSpecificResources();

    SafeRelease(&_CurveLineStyle);
    SafeRelease(&_CurveAreaStyle);
    SafeRelease(&_CurvePeakLineStyle);
    SafeRelease(&_CurvePeakAreaStyle);

    SafeRelease(&_BarAreaStyle);
    SafeRelease(&_BarTopStyle);
    SafeRelease(&_PeakAreaStyle);
    SafeRelease(&_PeakTopStyle);
    SafeRelease(&_DarkBackgroundStyle);
    SafeRelease(&_LightBackgroundStyle);

    SafeRelease(&_NyquistMarkerStyle);

    _OpacityMask.Release();
}
