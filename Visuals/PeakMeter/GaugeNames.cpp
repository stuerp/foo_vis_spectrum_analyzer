
/** $VER: GaugeNames.cpp (2024.05.01) P. Stuer - Implements the gauge names of the peak meter. **/

#include "framework.h"

#include "GaugeNames.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void GaugeNames::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void GaugeNames::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void GaugeNames::Reset()
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void GaugeNames::Resize() noexcept
{
    if (!_IsResized || (_Size.width == 0.f) || (_Size.height == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void GaugeNames::Render(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    if ((_GraphSettings->_XAxisMode == XAxisMode::None) || (!_GraphSettings->_XAxisTop && !_GraphSettings->_XAxisBottom))
        return;

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(renderTarget, gaugeMetrics);
    else
        RenderVertical(renderTarget, gaugeMetrics);
}

/// <summary>
/// Draws the channel names. Note: Rendered in absolute coordinates! No transformation.
/// </summary>
void GaugeNames::RenderHorizontal(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? _Size.height : 0.f;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, _Size.height);

        if (_TextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.left  = (_GraphSettings->_FlipHorizontally ? 0.f : GetWidth() - _TextStyle->GetWidth()) + Offset;
                Rect.right = _GraphSettings->_FlipHorizontally ? _TextStyle->GetWidth() : GetWidth();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.left  = (_GraphSettings->_FlipHorizontally ? GetWidth() - _TextStyle->GetWidth() : 0.f) + Offset;
                Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth()                          : _TextStyle->GetWidth();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Draws the channel names.
/// </summary>
void GaugeNames::RenderVertical(ID2D1RenderTarget * renderTarget, const GaugeMetrics & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? _Size.width: 0.f;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, _Size.width);

        if (_TextStyle->IsEnabled())
        {
            if (_GraphSettings->_XAxisTop)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? GetHeight() - _TextStyle->GetHeight() : 0.f;
                Rect.bottom = _GraphSettings->_FlipVertically ? GetHeight()                           : _TextStyle->GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphSettings->_XAxisBottom)
            {
                Rect.top    = _GraphSettings->_FlipVertically ? 0.f                     : GetHeight() - _TextStyle->GetHeight();
                Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->GetHeight() : GetHeight();

                renderTarget->DrawText(gv.Name.c_str(), (UINT) gv.Name.size(), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        Rect.left += _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT GaugeNames::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, renderTarget, Size, L"LFE", &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void GaugeNames::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}
