
/** $VER: SpectrumCurve.cpp (2026.11.23) P. Stuer - Implements a spectrum analyzer visualization **/

#include "pch.h"

#include "Spectrum.h"

#include "Direct2D.h"
#include "BezierSpline.h"
#include "FrequencyScaler.h"
#include "StyleManager.h"
#include "Support.h"

#pragma hdrstop

namespace Spectrum
{

/// <summary>
/// Renders the spectrum analysis as radial bars.
/// </summary>
void spectrum_t::RenderRadialBars(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State._IsPaused && !_State._VisualizeDuringPause)
        return;

    if (_Processor->FrequencyBands.empty())
        return;

    const FLOAT Side = std::min(_ClientSize.width / 2.f, _ClientSize.height / 2.f);
    const FLOAT InnerRadius =  Side * _State._InnerRadius;
    const FLOAT OuterRadius = (Side * _State._OuterRadius);// - InnerRadius;

    const FLOAT MaxSegmentHeight = OuterRadius - InnerRadius;

    FLOAT a = (FLOAT) ::fmod(Pi2 + (_AnimationTimer.Elapsed() * -DegreesToRadians(_State._AngularVelocity)), TwoPi);

    const FLOAT da = (FLOAT) TwoPi / (FLOAT) (_Processor->FrequencyBands.size() - 1);

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    size_t i = 0;
    double n = (double) _Processor->FrequencyBands.size();

    ComPtr<ID2D1PathGeometry> Path;

    ID2D1Brush * BarPeakAreaBrush = _BarPeakAreaStyle.Brush.Get();
    ID2D1Brush * BarPeakTopBrush  = _BarPeakTopStyle.Brush.Get();
    ID2D1Brush * BarAreaBrush     = _BarAreaStyle.Brush.Get();
    ID2D1Brush * BarTopBrush      = _BarTopStyle.Brush.Get();

    for (const auto & fb : _Processor->FrequencyBands)
    {
        const bool GreaterThanNyquist = fb.Low >= _Processor->NyquistFrequency; // 24/09/25: Use the lower frequency of a band instead of the center frequency.

        if (!GreaterThanNyquist || (GreaterThanNyquist && !_State._SuppressMirrorImage))
        {
            // Draw the peak indicator area.
            if (_BarPeakAreaStyle.IsEnabled())
            {
                const FLOAT r1 = InnerRadius;
                const FLOAT r2 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.PeakValue);

                if (SUCCEEDED(CreateSegment(a, a - da, r1, r2, Path.GetAddressOf())))
                {
                    if (_BarPeakAreaStyle.Has(style_t::Features::HorizontalGradient))
                    {
                        const double Value = _BarPeakAreaStyle.Has(style_t::Features::AmplitudeBasedColor) ? fb.Value : ((double) i / n);

                        _BarPeakAreaStyle.SetBrushColor(Value);
                    }

                    deviceContext->FillGeometry(Path.Get(), BarPeakAreaBrush);

                    Path.Reset();
                }
            }

            // Draw the peak indicator top.
            if (_BarPeakTopStyle.IsEnabled() &&(_State._PeakMode != PeakMode::None))// && (fb.MaxValue > 0.)) // Always draw the peak top indicator
            {
                const FLOAT r1 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.PeakValue) - _BarPeakTopStyle._Thickness / 2.f;
                const FLOAT r2 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.PeakValue) + _BarPeakTopStyle._Thickness;

                if (SUCCEEDED(CreateSegment(a, a - da, r1, r2, Path.GetAddressOf())))
                {
                    if (_BarPeakTopStyle.Has(style_t::Features::HorizontalGradient))
                    {
                        const double Value = _BarPeakTopStyle.Has(style_t::Features::AmplitudeBasedColor) ? fb.PeakValue : ((double) i / n);

                        _BarPeakTopStyle.SetBrushColor(Value);
                    }

                    const FLOAT Opacity = ((_State._PeakMode == PeakMode::FadeOut) || (_State._PeakMode == PeakMode::FadingAIMP)) ? (FLOAT) fb.Opacity : _BarPeakTopStyle._Opacity;

                    _BarPeakTopStyle.Brush->SetOpacity(Opacity);

                    deviceContext->FillGeometry(Path.Get(), BarPeakTopBrush);

                    Path.Reset();
                }
            }

            // Draw the area of the bar.
            if (_BarAreaStyle.IsEnabled())
            {
                const FLOAT r1 = InnerRadius;
                const FLOAT r2 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.Value);

                if (SUCCEEDED(CreateSegment(a, a - da, r1, r2, Path.GetAddressOf())))
                {
                    if (_BarAreaStyle.Has(style_t::Features::HorizontalGradient))
                    {
                        const double Value = _BarAreaStyle.Has(style_t::Features::AmplitudeBasedColor) ? fb.Value : ((double) i / n);

                        _BarAreaStyle.SetBrushColor(Value);
                    }

                    deviceContext->FillGeometry(Path.Get(), BarAreaBrush);

                    Path.Reset();
                }
            }

            // Draw the peak indicator top.
            if (_BarTopStyle.IsEnabled())
            {
                const FLOAT r1 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.Value) - _BarTopStyle._Thickness / 2.f;
                const FLOAT r2 = InnerRadius + (MaxSegmentHeight * (FLOAT) fb.Value) + _BarTopStyle._Thickness;

                if (SUCCEEDED(CreateSegment(a, a - da, r1, r2, Path.GetAddressOf())))
                {
                    if (_BarTopStyle.Has(style_t::Features::HorizontalGradient))
                    {
                        const double Value = _BarTopStyle.Has(style_t::Features::AmplitudeBasedColor) ? fb.PeakValue : ((double) i / n);

                        _BarTopStyle.SetBrushColor(Value);
                    }

                    deviceContext->FillGeometry(Path.Get(), BarTopBrush);

                    Path.Reset();
                }
            }
        }

        a -=da;
        ++i;
    }
}

/// <summary>
/// Creates a segment for the radial bar.
/// </summary>
HRESULT spectrum_t::CreateSegment(FLOAT a1, FLOAT a2, FLOAT r1, FLOAT r2, ID2D1PathGeometry ** pathGeometry) const noexcept
{
    HRESULT hr = Direct2DFactory::Get()->CreatePathGeometry(pathGeometry);

    if (FAILED(hr))
        return hr;

    ComPtr<ID2D1GeometrySink> Sink;

    hr = (*pathGeometry)->Open(Sink.GetAddressOf());

    if (FAILED(hr))
        return hr;

    FLOAT Sin, Cos;

    // Inner arc
    ::D2D1SinCos(a1, &Sin, &Cos);

    auto x = (FLOAT) (Cos * r1);
    auto y = (FLOAT) (Sin * r1);

    Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED);

    // Vertical from inner to outer arc.
    x = (FLOAT) (Cos * r2);
    y = (FLOAT) (Sin * r2);

    Sink->AddLine(D2D1::Point2F(x, y));

    // Outer arc
    ::D2D1SinCos(a2, &Sin, &Cos);

    x = (FLOAT) (Cos * r2);
    y = (FLOAT) (Sin * r2);

    Sink->AddArc(D2D1::ArcSegment(D2D1::Point2F(x, y), D2D1::SizeF(r2, r2), 0.0f, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));      

    // Vertical from outer to inner arc.
    x = (FLOAT) (Cos * r1);
    y = (FLOAT) (Sin * r1);

    Sink->AddLine(D2D1::Point2F(x, y));

    Sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    hr = Sink->Close();

    return hr;
}

/// <summary>
/// Renders the spectrum analysis as a radial curve.
/// </summary>
void spectrum_t::RenderRadialCurve(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State._IsPaused && !_State._VisualizeDuringPause)
        return;

    // Prepare the parameters.
    const FLOAT Side   = std::min(_ClientSize.width / 2.f, _ClientSize.height / 2.f);

    const FLOAT Width  = Side; //_ClientSize.width  / 2.f;
    const FLOAT Height = Side; //_ClientSize.height / 2.f;

    const FLOAT InnerRadiusX = Width  * _State._InnerRadius;
    const FLOAT InnerRadiusY = Height * _State._InnerRadius;

    double BeginAngle = DegreesToRadians(_State._BeginAngle);
    double EndAngle   = DegreesToRadians(_State._EndAngle);

    if (EndAngle <= BeginAngle)
        EndAngle += TwoPi;

    {
        const double AngleRange  = EndAngle - BeginAngle;
        const double OffsetAngle = _AnimationTimer.Elapsed() * DegreesToRadians(_State._AngularVelocity) * (_Clockwise ? -1. : 1.);

        BeginAngle = ::fmod(BeginAngle + OffsetAngle, TwoPi);

        if (BeginAngle < 0.)
            BeginAngle += TwoPi;

        EndAngle = _Clockwise ? BeginAngle - AngleRange : BeginAngle + AngleRange;
    }

    _Clockwise = (_State._AngularVelocity < 0.);

    // Start rendering.
    HRESULT hr = S_OK;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    geometry_points_t Points;

    ComPtr<ID2D1PathGeometry> Curve;

    if ((_State._PeakMode != PeakMode::None) && (_CurvePeakAreaStyle.IsEnabled() || _CurvePeakLineStyle.IsEnabled()))
    {
        Points.Clear();

        FLOAT a1 = (FLOAT) EndAngle;
        FLOAT a2 = (FLOAT) BeginAngle;

        hr = CreateRadialGeometryPointsFromAmplitude(Points, true, Width, Height, a1, a2);

        if (FAILED(hr))
            return;

        hr = CreateRadialCurve(Points, InnerRadiusX, InnerRadiusY, a1, a2, !_Clockwise, Curve.GetAddressOf());

        if (FAILED(hr))
            return;

        // Draw the area with the peak values.
        if (_CurvePeakAreaStyle.IsEnabled())
            deviceContext->FillGeometry(Curve.Get(), _CurvePeakAreaStyle.Brush.Get());

        // Draw the line with the peak values.
        if (_CurvePeakLineStyle.IsEnabled())
            deviceContext->DrawGeometry(Curve.Get(), _CurvePeakLineStyle.Brush.Get(), _CurvePeakLineStyle._Thickness);

        Curve.Reset();
    }

    if (_CurveAreaStyle.IsEnabled() || _CurveLineStyle.IsEnabled())
    {
        Points.Clear();

        FLOAT a1 = (FLOAT) EndAngle;
        FLOAT a2 = (FLOAT) BeginAngle;

        hr = CreateRadialGeometryPointsFromAmplitude(Points, false, Width, Height, a1, a2);

        if (FAILED(hr))
            return;

        hr = CreateRadialCurve(Points, InnerRadiusX, InnerRadiusY, a1, a2, !_Clockwise, Curve.GetAddressOf());

        if (FAILED(hr))
            return;

        // Draw the area with the current values.
        if (_CurveAreaStyle.IsEnabled())
            deviceContext->FillGeometry(Curve.Get(), _CurveAreaStyle.Brush.Get());

        // Draw the line with the current values.
        if (_CurveLineStyle.IsEnabled())
            deviceContext->DrawGeometry(Curve.Get(), _CurveLineStyle.Brush.Get(), _CurveLineStyle._Thickness);

        Curve.Reset();
    }
}

/// <summary>
/// Creates the geometry points from the amplitudes of the spectrum.
/// </summary>
HRESULT spectrum_t::CreateRadialGeometryPointsFromAmplitude(geometry_points_t & points, bool usePeak, FLOAT width, FLOAT height, FLOAT & beginAngle, FLOAT & endAngle) const noexcept
{
    if (_Processor->FrequencyBands.size() < 2)
        return E_FAIL;

    const FLOAT InnerRadiusX = width  * _State._InnerRadius;
    const FLOAT InnerRadiusY = height * _State._InnerRadius;

    const FLOAT OuterRadiusX = width  * _State._OuterRadius;
    const FLOAT OuterRadiusY = height * _State._OuterRadius;

    const FLOAT MaxWidth  = OuterRadiusX - InnerRadiusX;
    const FLOAT MaxHeight = OuterRadiusY - InnerRadiusY;

    const FLOAT AngleRange = (FLOAT) (endAngle - beginAngle);

    const FLOAT da = (AngleRange / (FLOAT) (_Processor->FrequencyBands.size() - 1)) * (_Clockwise ? -1.f : 1.f);

    size_t i = 0;

    // Create all the knots.
    for (const auto & fb: _Processor->FrequencyBands)
    {
        // Don't render anything above the Nyquist frequency.
        if ((fb.Low > _Processor->NyquistFrequency) && _State._SuppressMirrorImage)
            break;

        const double Value = !usePeak ? fb.Value : fb.PeakValue;

        const FLOAT RadiusX = InnerRadiusX + (MaxWidth  * (FLOAT) Value);
        const FLOAT RadiusY = InnerRadiusY + (MaxHeight * (FLOAT) Value);

        const FLOAT a = beginAngle + (FLOAT) i * da;

        FLOAT Sin, Cos;

        ::D2D1SinCos(a, &Sin, &Cos);

        const FLOAT x = (FLOAT) (Cos * RadiusX);
        const FLOAT y = (FLOAT) (Sin * RadiusY);

        points.p0.push_back(D2D1::Point2F(x, y));

        ++i;
    }

    if (points.p0.empty())
        return E_FAIL;

    // Create all the control points.
    const size_t n = points.p0.size();

    if (n > 1)
    {
        points.p1.reserve(n - 1);
        points.p2.reserve(n - 1);

        bezier_spline_t::GetControlPoints(points.p0, points.p1, points.p2);

        // Make sure all control points are on or above the inner ellipse.
        std::transform(points.p1.begin(), points.p1.end(), points.p1.begin(), [&](D2D1_POINT_2F & p)
        {
            const FLOAT x = (FLOAT) (p.x / InnerRadiusX);
            const FLOAT y = (FLOAT) (p.y / InnerRadiusY);
            const FLOAT d = (FLOAT) std::sqrt(x * x + y * y);

            if ((0.f < d) && (d < 1.f))
            {
                p.x = (FLOAT) (p.x / d);
                p.y = (FLOAT) (p.y / d);
            }

            return p;
        });

        std::transform(points.p2.begin(), points.p2.end(), points.p2.begin(), [&](D2D1_POINT_2F & p)
        {
            const FLOAT x = (FLOAT) (p.x / InnerRadiusX);
            const FLOAT y = (FLOAT) (p.y / InnerRadiusY);
            const FLOAT d = (FLOAT) std::sqrt(x * x + y * y);

            if ((0.f < d) && (d < 1.f))
            {
                p.x = (FLOAT) (p.x / d);
                p.y = (FLOAT) (p.y / d);
            }

            return p;
        });
    }

    return S_OK;
}

/// <summary>
/// Creates a radial curve from the power values.
/// </summary>
HRESULT spectrum_t::CreateRadialCurve(const geometry_points_t & gp, FLOAT innerRadiusX, FLOAT innerRadiusY, FLOAT beginAngle, FLOAT endAngle, bool clockwise, ID2D1PathGeometry ** curve) const noexcept
{
    *curve = nullptr;

    if ((gp.p0.size() < 2) || (innerRadiusX <= 0.f) || (innerRadiusY <= 0.f))
        return E_INVALIDARG;

    const FLOAT AngleRange = (FLOAT) std::fabs(endAngle - beginAngle);

    constexpr FLOAT AngleTolerance = 0.0001f;
    const bool IsFullCircle = (FLOAT) std::fabs(AngleRange - (FLOAT) TwoPi) <= AngleTolerance;

    if ((AngleRange <= 0.f) || (AngleRange > (FLOAT) TwoPi + AngleTolerance))
        return E_INVALIDARG;

    const D2D1_POINT_2F & OuterStart = gp.p0.front();

    // Determine the point on the inner ellipse at the same angle as the first point of the outer Bézier boundary.
    FLOAT BeginSin, BeginCos;

    ::D2D1SinCos(beginAngle, &BeginSin, &BeginCos);

    const D2D1_POINT_2F InnerStart = D2D1::Point2F((FLOAT) (BeginCos * innerRadiusX), (FLOAT) (BeginSin * innerRadiusY));

    D2D1_POINT_2F InnerEnd;

    if (!IsFullCircle)
    {
        FLOAT EndSin, EndCos;

        ::D2D1SinCos(endAngle, &EndSin, &EndCos);

        InnerEnd = D2D1::Point2F((FLOAT) (EndCos * innerRadiusX), (FLOAT) (EndSin * innerRadiusY));
    }
    else
        InnerEnd = InnerStart;

    ComPtr<ID2D1PathGeometry> Geometry;

    HRESULT hr = Direct2DFactory::Get()->CreatePathGeometry(Geometry.GetAddressOf());

    if (FAILED(hr))
        return hr;

    ComPtr<ID2D1GeometrySink> Sink;

    hr = Geometry->Open(Sink.GetAddressOf());

    if (FAILED(hr))
        return hr;

    Sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);

    Sink->BeginFigure(OuterStart, D2D1_FIGURE_BEGIN_FILLED); // Figure may take part in a fill operation.

    // Add the curve.
    const size_t n = gp.p1.size();

    for (size_t i = 0; i < n; ++i)
        Sink->AddBezier(D2D1::BezierSegment(gp.p1[i], gp.p2[i], gp.p0[i + 1]));

    // Add the line from the outer to the inner ellipse.
    Sink->AddLine(InnerEnd);

    // The inner ellipse is traversed in the opposite direction to close the area.
    const D2D1_SWEEP_DIRECTION SweepDirection = clockwise ? D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE : D2D1_SWEEP_DIRECTION_CLOCKWISE;

    if (IsFullCircle)
    {
        // Calculate the diametrically opposite point directly from InnerStart. This ensures that both half-arcs have stable and exactly opposing endpoints.
        const D2D1_POINT_2F InnerMiddle = D2D1::Point2F((FLOAT) -InnerStart.x, (FLOAT) -InnerStart.y);

        // A complete ellipse needs two arcs.
        Sink->AddArc(D2D1::ArcSegment(InnerMiddle, D2D1::SizeF(innerRadiusX, innerRadiusY), 0.f, SweepDirection, D2D1_ARC_SIZE_SMALL));
        Sink->AddArc(D2D1::ArcSegment(InnerStart,  D2D1::SizeF(innerRadiusX, innerRadiusY), 0.f, SweepDirection, D2D1_ARC_SIZE_SMALL));
    }
    else
    {
        const D2D1_ARC_SIZE ArcSize = (AngleRange < (FLOAT) Pi) ? D2D1_ARC_SIZE_SMALL : D2D1_ARC_SIZE_LARGE;

        // Add the inner ellipse segment.
        Sink->AddArc(D2D1::ArcSegment(InnerStart, D2D1::SizeF(innerRadiusX, innerRadiusY), 0.f, SweepDirection, ArcSize));
    }

    Sink->EndFigure(D2D1_FIGURE_END_CLOSED);

    hr = Sink->Close();

    if (SUCCEEDED(hr))
        *curve = Geometry.Detach();

    return hr;
}

}
