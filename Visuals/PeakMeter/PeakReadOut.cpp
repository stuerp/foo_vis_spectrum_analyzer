
/** $VER: PeakReadOut.cpp (2025.09.24) P. Stuer - Implements the peak read out of the peak meter. **/

#include "pch.h"

#include "PeakReadOut.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void peak_read_out_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void peak_read_out_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void peak_read_out_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void peak_read_out_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void peak_read_out_t::Render(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr) || !IsVisible())
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(deviceContext, gaugeMetrics);
    else
        RenderVertical(deviceContext, gaugeMetrics);
}

/// <summary>
/// Render this instance horizontally.
/// </summary>
void peak_read_out_t::RenderHorizontal(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphDescription->_FlipVertically ? GetHeight() : 0.f;

    const FLOAT dy = _GraphDescription->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, GetHeight());

        // Draw the peak text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.left  = _GraphDescription->_FlipHorizontally ? GetWidth() - _TextStyle->_Width : 0.f;
            Rect.right = _GraphDescription->_FlipHorizontally ? GetWidth()                      : _TextStyle->_Width;

            WCHAR Text[16];

            if (::isfinite(gv.Peak))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.Peak);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

 //         deviceContext->FillRectangle(Rect, _DebugBrush);
            deviceContext->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.top += _GraphDescription->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Render this instance vertically.
/// </summary>
void peak_read_out_t::RenderVertical(ID2D1DeviceContext * deviceContext, const gauge_metrics_t & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphDescription->_FlipHorizontally ? GetWidth(): 0.f;

    const FLOAT dx = _GraphDescription->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _TextStyle->SetVerticalAlignment(_GraphDescription->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, GetWidth());

        if (_TextStyle->IsEnabled())
        {

            Rect.top    = _GraphDescription->_FlipVertically ? 0.f                 : GetHeight() - _TextStyle->_Height;
            Rect.bottom = _GraphDescription->_FlipVertically ? _TextStyle->_Height : GetHeight();

            WCHAR Text[16];

            if (::isfinite(gv.Peak))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.Peak);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

//          deviceContext->FillRectangle(Rect, _DebugBrush);
            deviceContext->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.left += _GraphDescription->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT peak_read_out_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = deviceContext->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugePeakLevelText, deviceContext, Size, L"+199.9", 1.f, &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void peak_read_out_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_TextStyle)
    {
        _TextStyle->DeleteDeviceSpecificResources();
        _TextStyle = nullptr;
    }
}
