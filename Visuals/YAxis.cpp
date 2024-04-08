
/** $VER: YAXis.cpp (2024.04.06) P. Stuer - Implements the Y axis of a graph. **/

#include "framework.h"
#include "YAxis.h"

#include "StyleManager.h"
#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void YAxis::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    _FlipVertically = settings->_FlipVertically;

    _Labels.clear();

    if (_GraphSettings->_YAxisMode == YAxisMode::None)
        return;
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void YAxis::Move(const D2D1_RECT_F & rect)
{
    _Bounds = rect;
    _Size = { rect.right - rect.left, rect.bottom - rect.top };

    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void YAxis::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    // Create the labels.
    {
        _Labels.clear();

        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

            Label lb = { Amplitude, Text };

            _Labels.push_back(lb);
        }
    }

    const FLOAT xl = _Bounds.left  + (_GraphSettings->_YAxisLeft  ? _TextStyle->_TextWidth : 0.f); // Left axis
    const FLOAT xr = _Bounds.right - (_GraphSettings->_YAxisRight ? _TextStyle->_TextWidth : 0.f); // Right axis

    // Calculate the position of the labels based on the height.
    D2D1_RECT_F OldRect = {  };

    for (Label & Iter : _Labels)
    {
        FLOAT y = Map(_GraphSettings->ScaleA(ToMagnitude(Iter.Amplitude)), 0., 1., !_FlipVertically ? _Bounds.bottom : _Bounds.top, !_FlipVertically ? _Bounds.top : _Bounds.bottom);

        // Don't generate any labels outside the bounds.
        if (!InRange(y, _Bounds.top, _Bounds.bottom))
            continue;

        Iter.PointL = D2D1_POINT_2F(xl, y);
        Iter.PointR = D2D1_POINT_2F(xr, y);

        y -= (_TextStyle->_TextHeight / 2.f);

        if ((!_GraphSettings->_XAxisTop) && (y <_Bounds.top))
            y = _Bounds.top;

        if ((!_GraphSettings->_XAxisBottom) && (y + _TextStyle->_TextHeight > _Bounds.bottom))
            y = _Bounds.bottom - _TextStyle->_TextHeight;

        Iter.RectL = { _Bounds.left, y, xl - 2.f,      y + _TextStyle->_TextHeight };
        Iter.RectR = { xr + 2.f,     y, _Bounds.right, y + _TextStyle->_TextHeight };

        OldRect = Iter.RectL;
    }

    // Make sure  the first and last label are always drawn.
    if (_Labels.size() > 2)
    {
        if (InRange(_Labels[0].RectL.top, _Labels[1].RectL.top, _Labels[1].RectL.bottom))
            _Labels[1].Text.clear();

        if (InRange(_Labels.back().RectL.bottom, _Labels[_Labels.size() - 2].RectL.top, _Labels[_Labels.size() - 2].RectL.bottom))
            _Labels[_Labels.size() - 2].Text.clear();
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void YAxis::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); // Right-align horizontally

    for (const Label & Iter : _Labels)
    {
        // Draw the horizontal grid line.
        if (_LineStyle->IsEnabled())
            renderTarget->DrawLine(Iter.PointL, Iter.PointR, _LineStyle->_Brush, _LineStyle->_Thickness, nullptr);

        // Draw the labels.
        if (_TextStyle->IsEnabled() && (_GraphSettings->_YAxisMode != YAxisMode::None))
        {
            if (_GraphSettings->_YAxisLeft)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectL, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            if (_GraphSettings->_YAxisRight)
                renderTarget->DrawText(Iter.Text.c_str(), (UINT) Iter.Text.size(), _TextStyle->_TextFormat, Iter.RectR, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
    }
}

#pragma region DirectX

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// It's all centralized here, in case the resources need to be recreated in case of D3D device loss (eg. display change, remoting, removal of video card, etc).
/// </summary>
HRESULT YAxis::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, L"", &_LineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, L"+999", &_TextStyle);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void YAxis::ReleaseDeviceSpecificResources()
{
    SafeRelease(&_TextStyle);
    SafeRelease(&_LineStyle);
}

#pragma endregion
