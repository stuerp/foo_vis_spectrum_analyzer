
/** $VER: BitMeter.cpp (2026.10.27) P. Stuer - Implements a bit meter visualization. **/

#include <pch.h>

#include "BitMeter.h"

#pragma hdrstop

namespace BitMeter
{

/// <summary>
/// Destroys this instance.
/// </summary>
bit_meter_t::~bit_meter_t() noexcept
{
    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void bit_meter_t::Configure(graph_options_t * options, audio_processor_t * processor, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _GraphOptions = options;
    _Processor    = (processor_t *) processor;

    _MeasurementCount = 0;

    Reset();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void bit_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);
}

/// <summary>
/// Renders this instance.
/// </summary>
void bit_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    // Draw the static content.
    {
        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(_Rect.left, _Rect.top);

        deviceContext->SetTransform(Translate);

        deviceContext->DrawImage(_StaticContentCommandList.Get());
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

    const bool Visualize = !_State._IsPaused || _State._VisualizeDuringPause;
    const FLOAT TranslationX = _Rect.left + YAxisWidth + XOffset;
    const FLOAT ChannelOffset = ChannelHeight + XAxisHeight;

    D2D1_RECT_F r =
    {
        .top = 0.f,
        .bottom = ChannelHeight
    };

    for (const auto & Channel : _State._ChannelOrder)
    {
        const auto it = std::find_if(_Processor->_BitMeasurements.begin(), _Processor->_BitMeasurements.end(), [Channel](const auto & m){ return m.Channel == Channel; });

        if (it == _Processor->_BitMeasurements.end())
            continue;

        {
            const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(TranslationX, _Rect.top + YOffset);

            deviceContext->SetTransform(Translate);

            r.left = 0.f;

            // Draw the bit bar counts for the current channel.
            size_t BitNumber = 0;

            for (const auto & BitCount : it->BitCounts)
            {
                r.right = r.left + BarWidth - 1.f;

                if (Visualize)
                {
                    style_t * Style = _Styles[BitNumber];

                    if (Style->IsEnabled())
                    {
                        if (_State._OpacityMode)
                        {
                            r.top = 0.f;
                            Style->_Brush->SetOpacity((FLOAT) BitCount);
                        }
                        else
                        {
                            r.top = ChannelHeight - ((FLOAT) BitCount * ChannelHeight);
                            Style->_Brush->SetOpacity(Style->_Opacity); // Always set the opacity in case we're returning from opacity mode.
                        }

                        deviceContext->FillRectangle(r, Style->_Brush.Get());
                    }
                }

                r.left += BarWidth;
                ++BitNumber;
            }

            YOffset += ChannelOffset;
        }
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

/// <summary>
/// Resets this instance.
/// </summary>
void bit_meter_t::Reset() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT bit_meter_t::CreateDeviceSpecificResources(_In_ ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _DebugBrush.GetAddressOf());
#endif

    if (_MeasurementCount != _Processor->_BitMeasurements.size())
    {
        _MeasurementCount = _Processor->_BitMeasurements.size();

        _State._SizeHasChanged = true;
        _State._ColorsHaveChanged = true;
    }

    if (_State._ColorsHaveChanged || _State._SizeHasChanged)
    {
        DeleteColorDependentResources();

        Initialize();

        hr = CreateColorDependentResources(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    if (_State._SizeHasChanged)
    {
        DeleteSizeDependentResources();

        Initialize();

        hr = CreateSizeDependentResources(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void bit_meter_t::DeleteDeviceSpecificResources() noexcept
{
    DeleteColorDependentResources();
    DeleteSizeDependentResources();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Creates the resources that depend on the size of the render target.
/// </summary>
HRESULT bit_meter_t::CreateSizeDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_DeviceContext == nullptr)
    {
        ComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(D2DDevice.GetAddressOf());

        HRESULT hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, _DeviceContext.GetAddressOf());

        if (FAILED(hr))
            return hr;
    }

    if (_StaticContentCommandList == nullptr)
    {
        HRESULT hr = CreateStaticContentCommandList();

        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

/// <summary>
/// Deletes the resources that depend on the size of the render target.
/// </summary>
void bit_meter_t::DeleteSizeDependentResources() noexcept
{
    _StaticContentCommandList.Reset();

    _DeviceContext.Reset();
}

/// <summary>
/// Creates the resources that depend on the color palette.
/// </summary>
HRESULT bit_meter_t::CreateColorDependentResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_MeasurementCount == 0)
        return E_FAIL;

    const D2D1_SIZE_F TextSize = { _Size.width, _Size.height / (FLOAT) _MeasurementCount };

    const auto InitializeStyle = [this, deviceContext](auto & Style, VisualElement Element, const D2D1_SIZE_F & Size, const wchar_t * Text) noexcept -> HRESULT
    {
        if (Style._Brush != nullptr)
            return S_OK;

        Style = *_State._StyleManager.GetStyle(Element);

        Style.SetColor(&_State);

        return Style.CreateDeviceSpecificResources(deviceContext, Size, Text, 1.f);
    };

    HRESULT hr = InitializeStyle(_BarBackground, VisualElement::BarBackground, TextSize, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_BarSign, VisualElement::BarSign, TextSize, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_BarExponent, VisualElement::BarExponent, TextSize, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_BarMantissa, VisualElement::BarMantissa, TextSize, L"");

    if (FAILED(hr))
        return hr;

    if (_XAxisText._Brush == nullptr)
    {
        hr = InitializeStyle(_XAxisText, VisualElement::XAxisText, _Size, L"");

        if (FAILED(hr))
            return hr;

        _XAxisText.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        _XAxisText.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    if (_YAxisText._Brush == nullptr)
    {
        hr = InitializeStyle(_YAxisText, VisualElement::YAxisText, _Size, L"WW");

        if (FAILED(hr))
            return hr;

        _YAxisText.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    }

    // Predetermine the style for each bit.
    if ((_Styles.size() != _BitCount) || (_CachedBitMeterMode != _State._BitMeterMode))
    {
        _Styles.resize(_BitCount);

        for (size_t BitNumber = 0; BitNumber < _BitCount; ++BitNumber)
        {
            if (_State._BitMeterMode == BitMeterMode::FloatingPoint)
                _Styles[BitNumber] = (BitNumber == 0) ? &_BarSign : ((BitNumber <= ExponentBits) ? &_BarExponent : &_BarMantissa);
            else
                _Styles[BitNumber] = &_BarMantissa;
        }
    }

    return S_OK;
}

/// <summary>
/// Deletes the resources that depend on the color palette.
/// </summary>
void bit_meter_t::DeleteColorDependentResources() noexcept
{
    _YAxisText.DeleteDeviceSpecificResources();
    _XAxisText.DeleteDeviceSpecificResources();

    _BarMantissa.DeleteDeviceSpecificResources();
    _BarExponent.DeleteDeviceSpecificResources();
    _BarSign.DeleteDeviceSpecificResources();
    _BarBackground.DeleteDeviceSpecificResources();

    _Styles.clear();
}

/// <summary>
/// Creates a command list to render the static content.
/// </summary>
HRESULT bit_meter_t::CreateStaticContentCommandList() noexcept
{
    if (_MeasurementCount == 0)
        return E_FAIL;

    HRESULT hr = _DeviceContext->CreateCommandList(_StaticContentCommandList.GetAddressOf());

    if (FAILED(hr))
        return hr;

    _DeviceContext->SetTarget(_StaticContentCommandList.Get());

    _DeviceContext->BeginDraw();

    _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

    _DeviceContext->Clear(); // Transparent

    const bool DrawXAxis         = _GraphOptions->_XAxisBottom && _XAxisText.IsEnabled();
    const bool DrawYAxis         = _GraphOptions->_YAxisLeft   && _YAxisText.IsEnabled();
    const bool DrawBarBackground = _BarBackground.IsEnabled();

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

    for (const auto & Channel : _State._ChannelOrder)
    {
        const auto it = std::find_if(_Processor->_BitMeasurements.begin(), _Processor->_BitMeasurements.end(), [Channel](const auto & m){ return m.Channel == Channel; });

        if (it == _Processor->_BitMeasurements.end())
            continue;

        const auto & m = *it;

        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(0.f, YOffset);

        _DeviceContext->SetTransform(Translate);

        // Draw the channel name.
        {
            r.left = XOffset;

            if (DrawYAxis)
            {
                r.left  += XPadding;
                r.right = r.left + _YAxisText._Width;

//              _DeviceContext->DrawRectangle(r, _DebugBrush);
                _DeviceContext->DrawText(m.ChannelName.c_str(), (UINT) m.ChannelName.size(), _YAxisText._TextFormat.Get(), r, _YAxisText._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

                r.left = r.right + XPadding;
            }
        }

        // Draw the bit bar backgrounds and numbers.
        for (size_t BitNumber = 0; BitNumber < m.BitCounts.size(); ++BitNumber)
        {
            // Draw the background.
            r.right = r.left + BarWidth - 1.f;

            if (DrawBarBackground)
                _DeviceContext->FillRectangle(r, _BarBackground._Brush.Get());

            // Draw the bit number.
            if (DrawXAxis)
            {
                const std::wstring & Text = _Labels[BitNumber];

                const D2D1_RECT_F cr = { r.left, r.bottom, r.right, r.bottom + XAxisHeight };

//              _DeviceContext->DrawRectangle(cr, _DebugBrush);
                _DeviceContext->DrawText(Text.c_str(), (UINT) Text.size(), _XAxisText._TextFormat.Get(), cr, _XAxisText._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            r.left += BarWidth;
        }

        YOffset += ChannelHeight + XAxisHeight;
    }

    hr = _DeviceContext->EndDraw();

    if (FAILED(hr))
        return hr;

    hr = _StaticContentCommandList->Close();

    return hr;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void bit_meter_t::Initialize() noexcept
{
    _BitCount = (size_t) ((_State._BitMeterMode == BitMeterMode::FloatingPoint) ? audio_sample_size : _State._BitsPerInteger);

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
/// Handles a configuration change event.
/// </summary>
void bit_meter_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        _StaticContentCommandList.Reset();

    _State._SizeHasChanged = true;
}

}
