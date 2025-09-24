
/** $VER: RMSReadOut.cpp (2024.04.22) P. Stuer - Implements the RMS read out of the peak meter. **/

#include "pch.h"

#include "RMSReadOut.h"

#pragma hdrstop

/// <summary>
/// Initializes this instance.
/// </summary>
void rms_read_out_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance.
/// </summary>
void rms_read_out_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void rms_read_out_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void rms_read_out_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void rms_read_out_t::Render(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr) || !IsVisible())
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    if (_State->_HorizontalPeakMeter)
        RenderHorizontal(renderTarget, gaugeMetrics);
    else
        RenderVertical(renderTarget, gaugeMetrics);
}

/// <summary>
/// Render this instance horizontally.
/// </summary>
void rms_read_out_t::RenderHorizontal(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.top = _GraphSettings->_FlipVertically ? GetHeight() : 0.f;

    const FLOAT dy = _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight : gaugeMetrics._BarHeight;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    _TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.bottom = std::clamp(Rect.top + dy, 0.f, GetHeight());

        // Draw the RMS text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.left  = _GraphSettings->_FlipHorizontally ? GetWidth() - _TextStyle->_Width : 0.f;
            Rect.right = _GraphSettings->_FlipHorizontally ? GetWidth()                      : _TextStyle->_Width;

            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

//          renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.top += _GraphSettings->_FlipVertically ? -gaugeMetrics._BarHeight - _State->_GaugeGap : gaugeMetrics._BarHeight + _State->_GaugeGap;
    }
}

/// <summary>
/// Render this instance vertically.
/// </summary>
void rms_read_out_t::RenderVertical(ID2D1RenderTarget * renderTarget, const gauge_metrics_t & gaugeMetrics) const noexcept
{
    D2D1_RECT_F Rect = { };

    Rect.left = _GraphSettings->_FlipHorizontally ? GetWidth() : 0.f;

    const FLOAT dx = _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth : gaugeMetrics._BarWidth;

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    _TextStyle->SetVerticalAlignment(_GraphSettings->_FlipVertically ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    for (const auto & gv : _Analysis->_GaugeValues)
    {
        Rect.right = std::clamp(Rect.left + dx, 0.f, GetWidth());

        // Draw the RMS text display.
        if (_TextStyle->IsEnabled())
        {
            Rect.top    = _GraphSettings->_FlipVertically ? 0.f                 : GetHeight() - _TextStyle->_Height;
            Rect.bottom = _GraphSettings->_FlipVertically ? _TextStyle->_Height : GetHeight();

            WCHAR Text[16];

            if (::isfinite(gv.RMS))
                ::StringCchPrintfW(Text, _countof(Text), L"%+5.1f", gv.RMS);
            else
                ::wcscpy_s(Text, _countof(Text), NegativeInfinity);

//          renderTarget->FillRectangle(Rect, _DebugBrush);
            renderTarget->DrawText(Text, (UINT) ::wcslen(Text), _TextStyle->_TextFormat, Rect, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        Rect.left += _GraphSettings->_FlipHorizontally ? -gaugeMetrics._BarWidth - _State->_GaugeGap : gaugeMetrics._BarWidth + _State->_GaugeGap;
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT rms_read_out_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GaugeRMSLevelText, renderTarget, Size, L"+199.9", &_TextStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void rms_read_out_t::ReleaseDeviceSpecificResources() noexcept
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
