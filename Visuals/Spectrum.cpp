
/** $VER: Spectrum.cpp (2024.01.15) P. Stuer **/

#include "Spectrum.h"

#include "Direct2D.h"
#include "DirectWrite.h"

#include "BezierSpline.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void Spectrum::Initialize(const Configuration * configuration)
{
    _Configuration = configuration;

    SetGradientStops(_Configuration->_GradientStops);

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
void Spectrum::Render(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
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
void Spectrum::RenderBars(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    const FLOAT Width = _Bounds.right - _Bounds.left;
    const FLOAT Height = _Bounds.bottom - _Bounds.top;
    const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

    FLOAT x1 = _Bounds.left;
    FLOAT x2 = x1 + BandWidth;

    for (const FrequencyBand & Iter : frequencyBands)
    {
        D2D1_RECT_F Rect = { x1, _Bounds.top, x2 - PaddingX, Height - PaddingY };

        // Draw the bar background.
        if (_Configuration->_DrawBandBackground)
        {
            _SolidBrush->SetColor(Iter.BackColor);

            renderTarget->FillRectangle(Rect, _SolidBrush);
        }

        // Don't render bar foreground above the Nyquist frequency.
        if (Iter.Ctr < (sampleRate / 2.))
        {
            ID2D1Brush * ForeBrush = _Configuration->_HorizontalGradient ? _SolidBrush : (ID2D1Brush *) _GradientBrush;

            if (_Configuration->_HorizontalGradient)
                _SolidBrush->SetColor(Iter.ForeColor);

            // Draw the foreground.
            if (Iter.CurValue > 0.0)
            {
                Rect.top = Clamp((FLOAT)(_Bounds.bottom - (Height * _Configuration->ScaleA(Iter.CurValue))), _Bounds.top, _Bounds.bottom);

                renderTarget->FillRectangle(Rect, ForeBrush);

                if (_Configuration->_LEDMode)
                    renderTarget->FillRectangle(Rect, _PatternBrush);
            }

            // Draw the peak indicator.
            if ((_Configuration->_PeakMode != PeakMode::None) && (Iter.Peak > 0.))
            {
                Rect.top = Clamp((FLOAT)(_Bounds.bottom - (Height * Iter.Peak)), _Bounds.top, _Bounds.bottom);
                Rect.bottom = Rect.top + 1.f;

                ID2D1Brush * PeakBrush = (_Configuration->_PeakMode != PeakMode::FadeOut) ? ForeBrush : _WhiteBrush;

                if (_Configuration->_PeakMode == PeakMode::FadeOut)
                    PeakBrush->SetOpacity((FLOAT) Iter.Opacity);

                renderTarget->FillRectangle(Rect, PeakBrush);

                if (_Configuration->_PeakMode == PeakMode::FadeOut)
                    PeakBrush->SetOpacity(1.f);
            }
        }

        x1  = x2;
        x2 += BandWidth;
    }
}

/// <summary>
/// Renders the spectrum analysis as a curve.
/// </summary>
void Spectrum::RenderCurve(CComPtr<ID2D1HwndRenderTarget> & renderTarget, const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    HRESULT hr = CreateCurve(frequencyBands, sampleRate);

    if (SUCCEEDED(hr))
    {
        _GradientBrush->SetOpacity(_Configuration->_AreaOpacity);

        renderTarget->FillGeometry(_Curve, _GradientBrush);

        _GradientBrush->SetOpacity(1.0f);

        renderTarget->DrawGeometry(_Curve, _GradientBrush, _Configuration->_LineWidth);

        _Curve.Release();
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT Spectrum::CreateDeviceSpecificResources(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    HRESULT hr = S_OK;

    if ((_SolidBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_SolidBrush);

    if ((_BackBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(_Configuration->_DarkBandColor, &_BackBrush);

    if ((_WhiteBrush == nullptr) && SUCCEEDED(hr))
        hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &_WhiteBrush);

    if ((_GradientBrush == nullptr) && SUCCEEDED(hr))
        hr = CreateGradientBrush(renderTarget);

    if ((_PatternBrush == nullptr) && SUCCEEDED(hr))
        hr = CreatePatternBrush(renderTarget);

    return hr;
}

/// <summary>
/// Creates a gradient brush for rendering the bars.
/// </summary>
HRESULT Spectrum::CreateGradientBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
{
    if (_GradientStops.empty())
        return E_FAIL;

    CComPtr<ID2D1GradientStopCollection> Collection;

    HRESULT hr = renderTarget->CreateGradientStopCollection(&_GradientStops[0], (UINT32) _GradientStops.size(), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &Collection);

    if (SUCCEEDED(hr))
    {
        D2D1_SIZE_F Size = renderTarget->GetSize();

        hr = renderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.f, 0.f), D2D1::Point2F(0, Size.height)), Collection, &_GradientBrush);
    }

    return hr;
}

/// <summary>
/// Creates a pattern brush for rendering LED mode.
/// </summary>
HRESULT Spectrum::CreatePatternBrush(CComPtr<ID2D1HwndRenderTarget> & renderTarget)
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
/// Creates a curve from the power values.
/// </summary>
HRESULT Spectrum::CreateCurve(const std::vector<FrequencyBand> & frequencyBands, double sampleRate)
{
    if (frequencyBands.size() < 2)
        return E_FAIL;

    HRESULT hr = _Direct2D.Factory->CreatePathGeometry(&_Curve);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> _Sink;

        hr = _Curve->Open(&_Sink);

        if (SUCCEEDED(hr))
        {
            const FLOAT Width = _Bounds.right - _Bounds.left;
            const FLOAT Height = _Bounds.bottom - _Bounds.top;
            const FLOAT BandWidth = Max((Width / (FLOAT) frequencyBands.size()), 1.f);

            _Sink->SetFillMode(D2D1_FILL_MODE_WINDING);

            FLOAT x = _Bounds.left + BandWidth / 2.f; // Make sure the knots are nicely centered in the bar rectangle.

            _Sink->BeginFigure(D2D1::Point2F(x, _Bounds.bottom), D2D1_FIGURE_BEGIN_FILLED);

            // Start with a vertical line going up.
            FLOAT y = Clamp((FLOAT)(_Bounds.bottom - (Height * _Configuration->ScaleA(frequencyBands[0].CurValue))), _Bounds.top, _Bounds.bottom);

            _Sink->AddLine(D2D1::Point2F(x, y));

            size_t n = frequencyBands.size(); // Determine how many knots will be used to calculate control points

            std::vector<D2D1_POINT_2F> p0(n);
            std::vector<D2D1_POINT_2F> p1(n - 1);
            std::vector<D2D1_POINT_2F> p2(n - 1);

            n = 0;

            for (const auto & Iter : frequencyBands)
            {
                // Don't render anything above the Nyquist frequency.
                if (Iter.Ctr > (sampleRate / 2.))
                    break;

                y = Clamp((FLOAT)(_Bounds.bottom - (Height * _Configuration->ScaleA(Iter.CurValue))), _Bounds.top, _Bounds.bottom);

                p0[n++] = D2D1::Point2F(x, y);

                x += BandWidth;
            }

            if (n > 1)
            {
                BezierSpline::GetControlPoints(p0, n, p1, p2);

                for (size_t i = 0; i < (n - 1); ++i)
                {
                    p1[i].y = Clamp(p1[i].y, _Bounds.top, _Bounds.bottom);
                    p2[i].y = Clamp(p2[i].y, _Bounds.top, _Bounds.bottom);

                    _Sink->AddBezier(D2D1::BezierSegment(p1[i], p2[i], p0[i + 1]));
                }

                // End with a vertical line going down.
                _Sink->AddLine(D2D1::Point2F(p0[n - 1].x, _Bounds.bottom));
            }

            _Sink->EndFigure(D2D1_FIGURE_END_OPEN);

            _Sink->Close();
        }
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

    _BackBrush.Release();
    _SolidBrush.Release();
    _GradientBrush.Release();
}

/// <summary>
/// 
/// </summary>
void Spectrum::SetGradientStops(const std::vector<D2D1_GRADIENT_STOP> & gradientStops)
{
    _GradientStops = gradientStops;

    _GradientBrush.Release();
}
