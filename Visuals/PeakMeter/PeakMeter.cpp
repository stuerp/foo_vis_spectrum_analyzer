
/** $VER: PeakMeter.cpp (2026.10.08) P. Stuer - Represents a peak meter. **/

#include "pch.h"

#include "PeakMeter.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
peak_meter_t::peak_meter_t()
{
    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
peak_meter_t::~peak_meter_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void peak_meter_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void peak_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    InitializeMetrics(rect);

    _RenderedChannels = 0;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Resets this instance.
/// </summary>
void peak_meter_t::Reset() noexcept
{
    _RenderedChannels = 0;
    _ForceElementToResize = true;
}

/// <summary>
/// Renders this instance.
/// </summary>
void peak_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

//  deviceContext->DrawRectangle(_Rect, _DebugBrush);

    for (const auto & Part : _Parts)
        Part->Render();
}

/// <summary>
/// Creates the parts of this instance (e.g. after resizing or a change in channel configuration)
/// </summary>
void peak_meter_t::CreateParts() noexcept
{
    if (_GraphOptions->_YAxisLeft)
    {
        _Parts.push_back(std::make_unique<scale_t>
        (
            _State, _GraphOptions,
            _State->_IsHorizontalPeakMeter ? DWRITE_TEXT_ALIGNMENT_CENTER   : DWRITE_TEXT_ALIGNMENT_TRAILING,
            _State->_IsHorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        ));
    }

    bool IsFirstBar = true;

    {
        const bool ReverseLayout = _State->_IsHorizontalPeakMeter ? _GraphOptions->_FlipVertically : _GraphOptions->_FlipHorizontally;

        const auto AddMeter = [this, &IsFirstBar](const auto & channel)
        {
            const auto Measurement = std::find_if(_Analysis->_PeakMeasurements.cbegin(), _Analysis->_PeakMeasurements.cend(), [channel](const peak_measurement_t & item)
            {
                return item.Channel == channel;
            });

            if (Measurement != _Analysis->_PeakMeasurements.cend())
            {
                if (_State->_HasCenterScale && !IsFirstBar)
                    _Parts.push_back(std::make_unique<scale_t>(_State, _GraphOptions, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(std::make_unique<bar_t>(_State, _GraphOptions, std::addressof(*Measurement)));

                IsFirstBar = false;
            }
        };

        const auto & ChannelOrder = _State->_ChannelOrder;

        if (ReverseLayout)
        {
            std::for_each(ChannelOrder.crbegin(), ChannelOrder.crend(), AddMeter);
        }
        else
        {
            std::for_each(ChannelOrder.cbegin(), ChannelOrder.cend(), AddMeter);
        }
    }

    if (_GraphOptions->_YAxisRight)
    {
        _Parts.push_back(std::make_unique<scale_t>
        (
            _State, _GraphOptions,
            _State->_IsHorizontalPeakMeter ? DWRITE_TEXT_ALIGNMENT_CENTER    : DWRITE_TEXT_ALIGNMENT_LEADING,
            _State->_IsHorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        ));
    }
}

/// <summary>
/// Deletes the parts of this instance.
/// </summary>
void peak_meter_t::DeleteParts() noexcept
{
    for (auto & Part : _Parts)
        Part.reset();

    _Parts.clear();
}

/// <summary>
/// Measures the parts after the Direct 2D resources have been assigned.
/// </summary>
void peak_meter_t::MeasureParts(ID2D1DeviceContext * deviceContext) noexcept
{
    const FLOAT ScaleWidth  = _ScaleTextStyle._Width  + _TickSize;
    const FLOAT ScaleHeight = _ScaleTextStyle._Height + _TickSize;

    uint32_t BarCount = 0;

    FLOAT TotalScaleWidth  = 0.f;
    FLOAT TotalScaleHeight = 0.f;

    // Calculate how much space the scales occupy.
    for (auto & Part : _Parts)
    {
        Part->Bind
        (
            deviceContext,
            &_BarBackgroundStyle,
            &_PeakStyle,
            &_Peak0dBStyle,
            &_MaxPeakStyle,
            &_PeakTextStyle,
            &_RMSStyle,
            &_RMS0dBStyle,
            &_RMSTextStyle,
            &_NameStyle,
            &_ScaleTextStyle,
            &_ScaleLineStyle,
            _DebugBrush.Get(),
            _OpacityMask.Get()
        );

        auto Scale = dynamic_cast<scale_t *>(Part.get());

        if (Scale != nullptr)
        {
            if (_State->_IsHorizontalPeakMeter)
                TotalScaleHeight += Scale->IsCenter() ? _ScaleTextStyle._Height : ScaleHeight;
            else
                TotalScaleWidth  += Scale->IsCenter() ? _ScaleTextStyle._Width  : ScaleWidth;
        }
        else
            ++BarCount;
    }

    const FLOAT TotalBarGap = _State->_HasCenterScale ? 0.f : _State->_BarGap * (FLOAT) (BarCount - 1);

    FLOAT Offset = 0.f;
    FLOAT BarWidth = 0.f;
    FLOAT BarHeight = 0.f;

    // Calculate the width / height of a bar and the offset on the graph.
    {
        if (_State->_IsHorizontalPeakMeter)
        {
            BarHeight = (_Size.height - TotalScaleHeight - TotalBarGap) / (FLOAT) BarCount;

            if ((_State->_MaxBarSize != 0.f) && (BarHeight > _State->_MaxBarSize))
                BarHeight = _State->_MaxBarSize;

            const FLOAT TotalBarHeight = (BarHeight * (FLOAT) BarCount) + TotalBarGap;

            Offset = (_Size.height - TotalScaleHeight - TotalBarHeight) / 2.f;
        }
        else
        {
            BarWidth = (_Size.width  - TotalScaleWidth  - TotalBarGap) / (FLOAT) BarCount;

            if ((_State->_MaxBarSize != 0.f) && (BarWidth > _State->_MaxBarSize))
                BarWidth = _State->_MaxBarSize;

            const FLOAT TotalBarWidth  = (BarWidth  * (FLOAT) BarCount) + TotalBarGap;

            Offset = (_Size.width - TotalScaleWidth - TotalBarWidth) / 2.f;
        }
    }

    // Layout the meter parts.
    bool NeedGap = false;

    D2D1_RECT_F Rect = _Rect;

    if (_State->_IsHorizontalPeakMeter)
    {
        FLOAT y = Rect.top + Offset;

        for (auto & Part : _Parts)
        {
            auto * Scale = dynamic_cast<scale_t *>(Part.get());

            if (Scale != nullptr) // Scale
            {
                Rect.top    = y;
                Rect.bottom = y + _ScaleTextStyle._Height + (Scale->IsCenter() ? 0.f : _TickSize);

                NeedGap = false;
            }
            else // Bar
            {
                if (NeedGap)
                    y += _State->_BarGap;

                Rect.top    = y;
                Rect.bottom = y + BarHeight;

                NeedGap = true;
            }

            Part->InitializeMetrics(Rect);

            y += Rect.bottom - Rect.top;
        }
    }
    else
    {
        FLOAT x = _Rect.left + Offset;

        for (auto & Part : _Parts)
        {
            auto * Scale = dynamic_cast<scale_t *>(Part.get());

            // A scale determines its own width.
            if (Scale != nullptr)
            {
                Rect.left  = x;
                Rect.right = x + _ScaleTextStyle._Width + (Scale->IsCenter() ? 0.f : _TickSize);

                NeedGap = false;
            }
            // A bar's width is determined by the remaining graph area.
            else
            {
                if (NeedGap)
                    x += _State->_BarGap;

                Rect.left  = x;
                Rect.right = x + BarWidth;

                NeedGap = true;
            }

            Part->InitializeMetrics(Rect);

            x += Rect.right - Rect.left;
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT peak_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    auto & StyleManager = _GraphOptions->_UseLocalStyles ? _GraphOptions->_StyleManager : _State->_StyleManager;

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
        (void) deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), _DebugBrush.GetAddressOf());
#endif

    if (_OpacityMask == nullptr)
    {
        hr = CreateOpacityMask(deviceContext);

        if (FAILED(hr))
            return hr;
    }

    const auto InitializeStyle = [this, deviceContext, & StyleManager](auto & style, VisualElement visualElement, const wchar_t * text) noexcept -> HRESULT
    {
        if (style._Brush != nullptr)
            return S_OK;

        style = *StyleManager.GetStyle(visualElement);

        style.SetColor(_State);

        return style.CreateDeviceSpecificResources(deviceContext, _Size, text, 1.f);
    };

    hr = InitializeStyle(_BarBackgroundStyle, VisualElement::BarBackground, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_PeakStyle, VisualElement::BarPeakLevel, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_Peak0dBStyle, VisualElement::Bar0dBPeakLevel, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_PeakTextStyle, VisualElement::BarPeakLevelText, L"+199.9");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_MaxPeakStyle, VisualElement::BarMaxPeakLevel, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_RMSStyle, VisualElement::BarRMSLevel, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_RMS0dBStyle, VisualElement::Bar0dBRMSLevel, L"");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_RMSTextStyle, VisualElement::BarRMSLevelText, L"+199.9");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_NameStyle, VisualElement::XAxisText, L"LFE");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_ScaleTextStyle, VisualElement::YAxisText, L"+999");

    if (FAILED(hr))
        return hr;

    hr = InitializeStyle(_ScaleLineStyle, VisualElement::HorizontalGridLine, L"");

    if (FAILED(hr))
        return hr;

    if ((_RenderedChannels != _Analysis->_PeakActiveChannelMask) || _State->_ResizeResources)
    {
        DeleteParts();

        CreateParts();

        MeasureParts(deviceContext);

        _RenderedChannels = _Analysis->_PeakActiveChannelMask;
    }

    return S_OK;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void peak_meter_t::DeleteDeviceSpecificResources() noexcept
{
    for (const auto & Part : _Parts)
        Part->Unbind();

    DeleteParts();

    _BarBackgroundStyle.DeleteDeviceSpecificResources();

    _PeakStyle.DeleteDeviceSpecificResources();
    _Peak0dBStyle.DeleteDeviceSpecificResources();
    _MaxPeakStyle.DeleteDeviceSpecificResources();
    _PeakTextStyle.DeleteDeviceSpecificResources();

    _RMSStyle.DeleteDeviceSpecificResources();
    _RMS0dBStyle.DeleteDeviceSpecificResources();
    _RMSTextStyle.DeleteDeviceSpecificResources();

    _NameStyle.DeleteDeviceSpecificResources();

    _ScaleTextStyle.DeleteDeviceSpecificResources();
    _ScaleLineStyle.DeleteDeviceSpecificResources();

    _OpacityMask.Reset();

#ifdef _DEBUG
    _DebugBrush.Reset();
#endif
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT peak_meter_t::CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept
{
    ComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = deviceContext->CreateCompatibleRenderTarget(D2D1::SizeF(_Size.width, _Size.height), rt.GetAddressOf());

    if (FAILED(hr))
        return hr;

    rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    ComPtr<ID2D1SolidColorBrush> Brush;

    hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), Brush.GetAddressOf()); // Black parts will be masked out.

    if (FAILED(hr))
        return hr;

    rt->BeginDraw();

    rt->Clear(); // Transparent

    const FLOAT LEDSize = _State->_LEDLight + _State->_LEDGap;

    if (LEDSize > 0.f)
    {
        if (_State->_IsHorizontalPeakMeter)
        {
            for (FLOAT x = 0.f; x < _Size.width; x += LEDSize)
                rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDLight, _Size.height), Brush.Get());
        }
        else
        {
            for (FLOAT y = 0.f; y < _Size.height; y += LEDSize)
                rt->FillRectangle(D2D1::RectF(0.f, y, _Size.width, y + _State->_LEDLight), Brush.Get());
        }
    }

    hr = rt->EndDraw();

    if (FAILED(hr))
        return hr;

    hr = rt->GetBitmap(_OpacityMask.GetAddressOf());

    return hr;
}

/// <summary>
/// Handles a configuration change.
/// </summary>
void peak_meter_t::OnConfigurationChanged(ConfigurationChanges configurationChanges) noexcept
{
    if (!IsSet(configurationChanges, ConfigurationChanges::Layout))
        return;

    _State->_ResizeResources = true;
}
