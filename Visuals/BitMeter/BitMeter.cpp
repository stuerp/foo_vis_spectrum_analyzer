
/** $VER: BitMeter.cpp (2026.06.17) P. Stuer - Implements a bit meter visualization. **/

#include <pch.h>

#include "BitMeter.h"

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
void bit_meter_t::Initialize(state_t * state, graph_options_t * graphDescription, const analysis_t * analysis, bool isFirst, bool isLast) noexcept
{
    _State = state;
    _GraphOptions = graphDescription;
    _Analysis = analysis;

    _MeasurementCount = 0;

    _BitCount = (size_t) ((_State->_BitMeterMode == BitMeterMode::FloatingPoint) ? audio_sample_size : _State->_BitsPerInteger);

    // Create the labels.
    {
        _Labels.clear();

        for (uint32_t BitNumber = 1; BitNumber <= _BitCount; ++BitNumber)
        {
            WCHAR Text[4] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%u", BitNumber);

            _Labels.push_back(Text);
        }
    }
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

    _StaticContentCommandList = nullptr;

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

    // Draw the static content.
    {
        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(_Rect.left, _Rect.top);

        deviceContext->SetTransform(Translate);

        deviceContext->DrawImage(_StaticContentCommandList);
    }

    const FLOAT XAxisHeight = _GraphOptions->_XAxisBottom ? YPadding + _XAxisText._Height + YPadding : 1.f;
    const FLOAT YAxisWidth  = _GraphOptions->_YAxisLeft   ? XPadding + _YAxisText._Width  + XPadding : 0.f;

    const FLOAT ClientWidth  = _Size.width - YAxisWidth;
    const FLOAT ClientHeight = _Size.height - ((FLOAT) _MeasurementCount * XAxisHeight);

    FLOAT BarWidth = ClientWidth  / (FLOAT) _BitCount;

    // Use the full width of the graph?
    if (_GraphOptions->_HorizontalAlignment != HorizontalAlignment::Fit)
        BarWidth = std::floor(BarWidth);

    const FLOAT TotalBarWidth = BarWidth * (FLOAT) _BitCount;

    const FLOAT ChannelHeight = ClientHeight / (FLOAT) _MeasurementCount;

    const FLOAT XOffset = GetHOffset(_GraphOptions->_HorizontalAlignment, ClientWidth - TotalBarWidth);
    FLOAT YOffset = 0.f;

    // Draw the measurements for each selected channel.
    deviceContext->SetAntialiasMode( D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask() and results in crispier graphics.

    D2D1_RECT_F r = { .bottom = ChannelHeight };

    for (const auto & m : _Analysis->_BitMeasurements)
    {
        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(_Rect.left + YAxisWidth + XOffset, _Rect.top + YOffset);

        deviceContext->SetTransform(Translate);

        r.left = 0.f;

        // Draw the bit bar counts for the current channel.
        size_t BitNumber = 0;

        for (const auto & BitCount : m.BitCounts)
        {
            r.right = r.left + BarWidth - 1.f;

            if (!_State->_IsPaused || (_State->_IsPaused && _State->_VisualizeDuringPause))
            {
                style_t * Style = _Styles[BitNumber];

                if (Style->IsEnabled())
                {
                    if (_State->_OpacityMode)
                        Style->_Brush->SetOpacity((FLOAT) BitCount);
                    else
                        r.top = ChannelHeight - ((FLOAT) BitCount * ChannelHeight);

                    deviceContext->FillRectangle(r, Style->_Brush);
                }
            }

            r.left = r.right + 1.f;
            ++BitNumber;
        }

        YOffset += ChannelHeight + XAxisHeight;
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT bit_meter_t::CreateDeviceSpecificResources(_In_ ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width == 0.f) || _Size.height == 0.f)
        return E_FAIL;

    Resize();

    if (_MeasurementCount != _Analysis->_BitMeasurements.size())
    {
        _MeasurementCount = _Analysis->_BitMeasurements.size();

        _BarBackground.DeleteDeviceSpecificResources();
        _BarSign.DeleteDeviceSpecificResources();
        _BarExponent.DeleteDeviceSpecificResources();
        _BarMantissa.DeleteDeviceSpecificResources();

        _StaticContentCommandList.Release();
    }

    if (_MeasurementCount == 0)
        return E_FAIL;

    HRESULT hr = S_OK;

    const D2D1_SIZE_F TextSize = { _Size.width, _Size.height / (FLOAT) _MeasurementCount };

    if (_BarBackground._Brush == nullptr)
    {
        _BarBackground = *_State->_StyleManager.GetStyle(VisualElement::BarBackground);

        _BarBackground.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BarBackground.CreateDeviceSpecificResources(deviceContext, TextSize, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_BarSign._Brush == nullptr)
    {
        _BarSign = *_State->_StyleManager.GetStyle(VisualElement::BarSign);

        _BarSign.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BarSign.CreateDeviceSpecificResources(deviceContext, TextSize, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_BarExponent._Brush == nullptr)
    {
        _BarExponent = *_State->_StyleManager.GetStyle(VisualElement::BarExponent);

        _BarExponent.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BarExponent.CreateDeviceSpecificResources(deviceContext, TextSize, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_BarMantissa._Brush == nullptr)
    {
        _BarMantissa = *_State->_StyleManager.GetStyle(VisualElement::BarMantissa);

        _BarMantissa.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BarMantissa.CreateDeviceSpecificResources(deviceContext, TextSize, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_XAxisText._Brush == nullptr)
    {
        _XAxisText = *_State->_StyleManager.GetStyle(VisualElement::XAxisText);

        _XAxisText.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _XAxisText.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;

        _XAxisText.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _XAxisText.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    if (_YAxisText._Brush == nullptr)
    {
        _YAxisText = *_State->_StyleManager.GetStyle(VisualElement::YAxisText);

        _YAxisText.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _YAxisText.CreateDeviceSpecificResources(deviceContext, _Size, L"WW", 1.f);

        if (!SUCCEEDED(hr))
            return hr;

        _YAxisText.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
    {
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);

        if (!SUCCEEDED(hr))
            return hr;
    }
#endif

    if (_DeviceContext == nullptr)
    {
        CComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(&D2DDevice);

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_DeviceContext);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_StaticContentCommandList == nullptr)
    {
        hr = CreateStaticContentCommandList();


        if (!SUCCEEDED(hr))
            return hr;
    }

    // Predetermine the style for each bit.
    _Styles.resize(_BitCount);

    for (size_t BitNumber = 0; BitNumber < _BitCount; ++BitNumber)
    {
        if (_State->_BitMeterMode == BitMeterMode::FloatingPoint)
            _Styles[BitNumber] = (BitNumber == 0) ? &_BarSign : ((BitNumber <= ExponentBits) ? &_BarExponent : &_BarMantissa);
        else
            _Styles[BitNumber] = &_BarMantissa;
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void bit_meter_t::DeleteDeviceSpecificResources() noexcept
{
    _StaticContentCommandList.Release();

    _DeviceContext.Release();

#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    _YAxisText.DeleteDeviceSpecificResources();
    _XAxisText.DeleteDeviceSpecificResources();

    _BarMantissa.DeleteDeviceSpecificResources();
    _BarExponent.DeleteDeviceSpecificResources();
    _BarSign.DeleteDeviceSpecificResources();
    _BarBackground.DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates a command list to render the static content.
/// </summary>
HRESULT bit_meter_t::CreateStaticContentCommandList() noexcept
{
    HRESULT hr = _DeviceContext->CreateCommandList(&_StaticContentCommandList);

    if (!SUCCEEDED(hr))
        return hr;

    _DeviceContext->SetTarget(_StaticContentCommandList);
    _DeviceContext->BeginDraw();

    _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

    _DeviceContext->Clear(D2D1::ColorF(0, 0.f));

    const FLOAT XAxisHeight = _GraphOptions->_XAxisBottom ? YPadding + _XAxisText._Height + YPadding : 1.f;
    const FLOAT YAxisWidth  = _GraphOptions->_YAxisLeft   ? XPadding + _YAxisText._Width  + XPadding : 0.f;

    const FLOAT ClientWidth  = _Size.width - YAxisWidth;
    const FLOAT ClientHeight = _Size.height - ((FLOAT) _MeasurementCount * XAxisHeight);

    FLOAT BarWidth = ClientWidth  / (FLOAT) _BitCount;

    // Use the full width of the graph?
    if (_GraphOptions->_HorizontalAlignment != HorizontalAlignment::Fit)
        BarWidth = std::floor(BarWidth);

    const FLOAT TotalBarWidth = BarWidth * (FLOAT) _BitCount;

    const FLOAT ChannelHeight = ClientHeight / (FLOAT) _MeasurementCount;

    const FLOAT XOffset = GetHOffset(_GraphOptions->_HorizontalAlignment, ClientWidth - TotalBarWidth);
    FLOAT YOffset = 0.f;

    // Draw the static content for each selected channel.
    D2D1_RECT_F r = { .bottom = ChannelHeight };

    for (const auto & m : _Analysis->_BitMeasurements)
    {
        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(0.f, YOffset);

        _DeviceContext->SetTransform(Translate);

        // Draw the channel name.
        {
            r.left = XOffset;

            if (_GraphOptions->_YAxisLeft && _YAxisText.IsEnabled())
            {
                r.left  += XPadding;
                r.right = r.left + _YAxisText._Width;

//              _DeviceContext->DrawRectangle(r, _DebugBrush);
                _DeviceContext->DrawText(m.ChannelName.c_str(), (UINT) m.ChannelName.size(), _YAxisText._TextFormat, r, _YAxisText._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                r.left = r.right + XPadding;
            }
        }

        // Draw the bit bar backgrounds and numbers.
        for (size_t BitNumber = 0; BitNumber < m.BitCounts.size(); ++BitNumber)
        {
            // Draw the background.
            r.right = r.left + BarWidth - 1.f;

            if (_BarBackground.IsEnabled())
                _DeviceContext->FillRectangle(r, _BarBackground._Brush);

            // Draw the bit number.
            if (_GraphOptions->_XAxisBottom && _XAxisText.IsEnabled())
            {
                const std::wstring & Text = _Labels[BitNumber];

                const D2D1_RECT_F cr = { r.left, r.bottom, r.right, r.bottom + XAxisHeight };

//              _DeviceContext->DrawRectangle(cr, _DebugBrush);
                _DeviceContext->DrawText(Text.c_str(), (UINT) Text.size(), _XAxisText._TextFormat, cr, _XAxisText._Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            r.left = r.right + 1.f;
        }

        YOffset += ChannelHeight + XAxisHeight;
    }

    hr = _DeviceContext->EndDraw();

    if (!SUCCEEDED(hr))
        return hr;

    hr = _StaticContentCommandList->Close();

    return hr;
}

/// <summary>
/// Handles a configuration change event.
/// </summary>
void bit_meter_t::OnConfigurationChange(ConfigurationChanges configurationChanges) noexcept
{
    if ((configurationChanges & ConfigurationChanges::Layout) == ConfigurationChanges::Layout)
        _StaticContentCommandList.Release();
}
