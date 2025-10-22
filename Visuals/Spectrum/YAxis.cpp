
/** $VER: YAXis.cpp (2025.09.24) P. Stuer - Implements the Y axis of a graph. **/

#include "pch.h"
#include "YAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void y_axis_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    _FlipVertically = settings->_FlipVertically;

    _Labels.clear();

    if (_GraphDescription->_YAxisMode == YAxisMode::None)
        return;

    // Create the labels.
    {
        for (double Amplitude = _GraphDescription->_AmplitudeLo; Amplitude <= _GraphDescription->_AmplitudeHi; Amplitude -= _GraphDescription->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

            label_t lb = { Text, Amplitude };

            _Labels.push_back(lb);
        }
    }
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void y_axis_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void y_axis_t::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    const FLOAT xl = _Rect.left  + (_GraphDescription->_YAxisLeft  ? _TextStyle->_Width : 0.f); // Left axis
    const FLOAT xr = _Rect.right - (_GraphDescription->_YAxisRight ? _TextStyle->_Width : 0.f); // Right axis

    // Calculate the position of the labels based on the height.
    D2D1_RECT_F OldRect = {  };

    for (label_t & Iter : _Labels)
    {
        FLOAT y = msc::Map(_GraphDescription->ScaleAmplitude(ToMagnitude(Iter.Amplitude)), 0., 1., !_FlipVertically ? _Rect.bottom : _Rect.top, !_FlipVertically ? _Rect.top : _Rect.bottom);

        // Don't generate any labels outside the client rectangle.
        if (!msc::InRange(y, _Rect.top, _Rect.bottom))
        {
            Iter.IsHidden = true;
            continue;
        }

        Iter.PointL = D2D1_POINT_2F(xl, y);
        Iter.PointR = D2D1_POINT_2F(xr, y);

        y -= (_TextStyle->_Height / 2.f);

        if ((!_GraphDescription->_XAxisTop) && (y <_Rect.top))
            y = _Rect.top;

        if ((!_GraphDescription->_XAxisBottom) && (y + _TextStyle->_Height > _Rect.bottom))
            y = _Rect.bottom - _TextStyle->_Height;

        Iter.RectL = { _Rect.left, y, xl - 2.f,      y + _TextStyle->_Height };
        Iter.RectR = { xr + 2.f,     y, _Rect.right, y + _TextStyle->_Height };

        Iter.IsHidden = (Iter.Amplitude != _Labels.front().Amplitude) && (Iter.Amplitude != _Labels.back().Amplitude) && IsOverlappingVertically(Iter.RectL, OldRect);

        if (!Iter.IsHidden)
            OldRect = Iter.RectL;
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void y_axis_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); // Right-align horizontally, also for the right axis.

    for (const label_t & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        if (_LineStyle->IsEnabled())
            deviceContext->DrawLine(Iter.PointL, Iter.PointR, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

        // Draw the text.
        if (!Iter.IsHidden && _TextStyle->IsEnabled() && (_GraphDescription->_YAxisMode != YAxisMode::None))
        {
            if (_GraphDescription->_YAxisLeft)
                deviceContext->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectL, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if (_GraphDescription->_YAxisRight)
                deviceContext->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectR, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT y_axis_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_LineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"+999", 1.f, &_TextStyle);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void y_axis_t::DeleteDeviceSpecificResources() noexcept
{
    SafeRelease(&_TextStyle);
    SafeRelease(&_LineStyle);
}

#pragma endregion
