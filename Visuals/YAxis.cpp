
/** $VER: YAXis.cpp (2024.02.24) P. Stuer - Implements the Y axis of a graph. **/

#include "YAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void YAxis::Initialize(State * state, const GraphSettings * settings) noexcept
{
    _State = state;
    _GraphSettings = settings;

    _FlipVertically = settings->_FlipVertically;

    CreateDeviceIndependentResources();

    _Labels.clear();

    if (_GraphSettings->_YAxisMode == YAxisMode::None)
        return;

    // Precalculate the labels and their position.
    {
        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%ddB", (int) Amplitude);

            Label lb = { Amplitude, Text };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void YAxis::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;

    const FLOAT xl = _Bounds.left  + (_GraphSettings->_YAxisLeft  ? _Width : 0.f); // Left axis
    const FLOAT xr = _Bounds.right - (_GraphSettings->_YAxisRight ? _Width : 0.f); // Right axis

    // Calculate the position of the labels based on the height.
    for (Label & Iter : _Labels)
    {
        FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_FlipVertically ? _Bounds.bottom : _Bounds.top, !_FlipVertically ? _Bounds.top : _Bounds.bottom);

        // Don't generate any labels outside the bounds.
        if (!InRange(y, _Bounds.top, _Bounds.bottom))
            continue;

        Iter.PointL = D2D1_POINT_2F(xl, y);
        Iter.PointR = D2D1_POINT_2F(xr, y);

        y -= (_Height / 2.f);

        if ((!_GraphSettings->_XAxisTop) && (y <_Bounds.top))
            y = _Bounds.top;

        if ((!_GraphSettings->_XAxisBottom) && (y + _Height > _Bounds.bottom))
            y = _Bounds.bottom - _Height;

        Iter.RectL = { _Bounds.left, y, xl - 2.f,      y + _Height };
        Iter.RectR = { xr + 2.f,     y, _Bounds.right, y + _Height };
    }
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void YAxis::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    D2D1_RECT_F OldRect = {  };

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        if (_LineStyle->_ColorSource != ColorSource::None)
            renderTarget->DrawLine(Iter.PointL, Iter.PointR, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

        if ((_GraphSettings->_YAxisMode == YAxisMode::None) || (!_GraphSettings->_YAxisLeft && !_GraphSettings->_YAxisRight))
            continue;

        // Prevent overdraw of the labels.
        if (!InRange(Iter.RectL.top, OldRect.top, OldRect.bottom) && !InRange(Iter.RectL.bottom, OldRect.top, OldRect.bottom) && (_TextStyle->_ColorSource != ColorSource::None))
        {
            // Draw the labels.
            if (_GraphSettings->_YAxisLeft)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, Iter.RectL, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if (_GraphSettings->_YAxisRight)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextFormat, Iter.RectR, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            OldRect = Iter.RectL;
        }
    }
}

#pragma region DirectX

/// <summary>
/// Creates resources which are not bound to any D3D device.
/// </summary>
HRESULT YAxis::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    if (_TextFormat == 0)
    {
        const FLOAT FontSize = ToDIPs(_FontSize); // In DIP

        hr = _DirectWrite.Factory->CreateTextFormat(_FontFamilyName.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, FontSize, L"", &_TextFormat);

        if (SUCCEEDED(hr))
        {
            _TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);          // Right-align horizontally
            _TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);  // Center vertically

            CComPtr<IDWriteTextLayout> TextLayout;

            hr = _DirectWrite.Factory->CreateTextLayout(L"AaGg09", 6, _TextFormat, 100.f, 100.f, &TextLayout);

            if (SUCCEEDED(hr))
            {
                DWRITE_TEXT_METRICS TextMetrics = { };

                TextLayout->GetMetrics(&TextMetrics);

                _Height = TextMetrics.height;
            }
        }
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void YAxis::ReleaseDeviceIndependentResources()
{
    _TextFormat.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT YAxis::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = S_OK;

    const D2D1_SIZE_F Size = renderTarget->GetSize();

    {
        if (SUCCEEDED(hr))
        {
            if (_LineStyle == nullptr)
                _LineStyle = _State->_StyleManager.GetStyle(VisualElement::HorizontalGridLine);

            if (_LineStyle && (_LineStyle->_Brush == nullptr))
                hr = _LineStyle->CreateDeviceSpecificResources(renderTarget, Size);
        }
    }
    {
        if (SUCCEEDED(hr))
        {
            if (_TextStyle == nullptr)
                _TextStyle = _State->_StyleManager.GetStyle(VisualElement::YAxisText);

            if (_TextStyle && (_TextStyle->_Brush == nullptr))
                hr = _TextStyle->CreateDeviceSpecificResources(renderTarget, Size);
        }
    }

    if (SUCCEEDED(hr))
        renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE); // https://learn.microsoft.com/en-us/windows/win32/direct2d/improving-direct2d-performance

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void YAxis::ReleaseDeviceSpecificResources()
{
    _TextStyle = nullptr;
    _LineStyle = nullptr;

    for (const auto & Iter : { VisualElement::HorizontalGridLine, VisualElement::YAxisText })
    {
        Style * style = _State->_StyleManager.GetStyle(Iter);

        style->ReleaseDeviceSpecificResources();
    }
}

#pragma endregion
