
/** $VER: BitMeter.cpp (2026.03.11) P. Stuer - Implements a bit meter visualization. **/

#include <pch.h>

#include "BitMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
bit_meter_t::bit_meter_t()
{
    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
bit_meter_t::~bit_meter_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void bit_meter_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _Settings = settings;
    _Analysis = analysis;

    _MeasurementCount = 0;
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void bit_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void bit_meter_t::Reset() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void bit_meter_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void bit_meter_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void bit_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT XAxisHeight = _Settings->_XAxisBottom ? YPadding + _XAxisText->_Height + YPadding : 1.f;
    const FLOAT YAxisWidth  = _Settings->_YAxisLeft   ? XPadding + _YAxisText->_Width  + XPadding : 0.f;

    const FLOAT ClientWidth  = _Size.width - YAxisWidth;
    const FLOAT ClientHeight = _Size.height - ((FLOAT) _MeasurementCount * XAxisHeight);

    FLOAT dx = ClientWidth  / audio_sample_size;

    // Use the full width of the graph?
    if (_Settings->_HorizontalAlignment != HorizontalAlignment::Fit)
        dx = std::floor(dx);

    const FLOAT BarsWidth = dx * (FLOAT) audio_sample_size;

    const FLOAT dy = ClientHeight / (FLOAT) _MeasurementCount;

    #if (audio_sample_size == 64)
    constexpr size_t ExponentBits = 11;
    #else
    constexpr size_t ExponentBits =  8;
    #endif

    const FLOAT XOffset = GetHOffset(_Settings->_HorizontalAlignment, ClientWidth - BarsWidth);
    FLOAT YOffset = 0.f;

    D2D1_RECT_F r = { .bottom = dy };

    // Render the measurements for each selected channel.
    deviceContext->SetAntialiasMode( D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask() and results in crispier graphics.

    for (const auto & m : _Analysis->_BitMeasurements)
    {
        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(0.f, YOffset);

        deviceContext->SetTransform(Translate);

        // Draw the channel name.
        {
            r.top = 0.f;
            r.left = XOffset;

            if (_Settings->_YAxisLeft && _YAxisText->IsEnabled())
            {
                r.left  += XPadding;
                r.right = r.left + _YAxisText->_Width;

//              deviceContext->DrawRectangle(r, _DebugBrush);
                deviceContext->DrawText(m.ChannelName.c_str(), (UINT) m.ChannelName.size(), _YAxisText->_TextFormat, r, _YAxisText->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                r.left = r.right + XPadding;
            }
        }

        size_t BitNumber = 0;

        for (const auto & BitCount : m.BitCounts)
        {
            // Draw the background.
            r.top   = 0.f;
            r.right = r.left + dx - 1.f;

            if (_BarBackground->IsEnabled())
                deviceContext->FillRectangle(r, _BarBackground->_Brush);

            // Draw the bit count.
            if (!_State->_IsPaused || (_State->_IsPaused && _State->_VisualizeDuringPause))
            {
                style_t * Style = (BitNumber == 0) ? _BarSign : ((BitNumber <= ExponentBits) ? _BarExponent : _BarMantissa);

                if (Style->IsEnabled())
                {
                    r.top = dy - ((FLOAT) BitCount * dy);

                    deviceContext->FillRectangle(r, Style->_Brush);
                }
            }

            // Draw the bit number.
            if (_Settings->_XAxisBottom && _XAxisText->IsEnabled())
            {
                const std::wstring Text = msc::UTF8ToWide(pfc::format_int((t_int64) BitNumber + 1).c_str());

                const D2D1_RECT_F cr = { r.left, r.bottom, r.right, r.bottom + XAxisHeight };

//              deviceContext->DrawRectangle(cr, _DebugBrush);
                deviceContext->DrawText(Text.c_str(), (UINT) Text.size(), _XAxisText->_TextFormat, cr, _XAxisText->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            r.left = r.right + 1.f;
            ++BitNumber;
        }

        YOffset += dy + XAxisHeight;
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT bit_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width == 0.f) || _Size.height == 0.f)
        return E_FAIL;

    if (_MeasurementCount != _Analysis->_BitMeasurements.size())
    {
        _MeasurementCount = _Analysis->_BitMeasurements.size();

        SafeRelease(&_BarBackground);
        SafeRelease(&_BarSign);
        SafeRelease(&_BarExponent);
        SafeRelease(&_BarMantissa);
    }

    if (_MeasurementCount == 0)
        return E_FAIL;

    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarBackground, deviceContext, { _Size.width, _Size.height / (FLOAT) _MeasurementCount }, L"", 1.f, &_BarBackground);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarSign, deviceContext, { _Size.width, _Size.height / (FLOAT) _MeasurementCount }, L"", 1.f, &_BarSign);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarExponent, deviceContext, { _Size.width, _Size.height / (FLOAT) _MeasurementCount }, L"", 1.f, &_BarExponent);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarMantissa, deviceContext, { _Size.width, _Size.height / (FLOAT) _MeasurementCount }, L"", 1.f, &_BarMantissa);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, deviceContext, _Size, L"99", 1.f, &_XAxisText);

    if (SUCCEEDED(hr))
    {
        _XAxisText->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _XAxisText->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"WW", 1.f, &_YAxisText);

    if (SUCCEEDED(hr))
        _YAxisText->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void bit_meter_t::DeleteDeviceSpecificResources() noexcept
{
    SafeRelease(&_BarBackground);
    SafeRelease(&_BarSign);
    SafeRelease(&_BarExponent);
    SafeRelease(&_BarMantissa);

    SafeRelease(&_XAxisText);
    SafeRelease(&_YAxisText);

#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}
