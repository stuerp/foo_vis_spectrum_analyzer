
/** $VER: PeakMeter.cpp (2026.06.15) P. Stuer - Represents a peak meter. **/

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
void peak_meter_t::Initialize(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast) noexcept
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
    SetRect(rect);

    _RenderedChannels = 0;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Resets this instance.
/// </summary>
void peak_meter_t::Reset() noexcept
{
    _RenderedChannels = 0;
    _IsResized = true;
}

/// <summary>
/// Renders this instance.
/// </summary>
void peak_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

//  deviceContext->DrawRectangle(_Rect, _DebugBrush);

    for (auto Part : _Parts)
        Part->Render();
}

/// <summary>
/// Creates the parts of this instance (e.g. after resizing or a change in channel configuration)
/// </summary>
void peak_meter_t::CreateParts() noexcept
{
    if (_GraphOptions->_YAxisLeft)
    {
        _Parts.push_back(new scale_t
        (
            _State, _GraphOptions,
            _State->_IsHorizontalPeakMeter ? DWRITE_TEXT_ALIGNMENT_CENTER: DWRITE_TEXT_ALIGNMENT_TRAILING,
            _State->_IsHorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        ));
    }

    bool IsFirstBar = true;

    if (_State->_IsHorizontalPeakMeter)
    {
        if (_GraphOptions->_FlipVertically)
        {
            for (auto Measurement = _Analysis->_PeakMeasurements.rbegin(); Measurement != _Analysis->_PeakMeasurements.rend(); ++Measurement)
            {
                if (_State->_HasCenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _GraphOptions, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _GraphOptions, &(*Measurement)));

                IsFirstBar = false;
            }
        }
        else
        {
            for (auto Measurement = _Analysis->_PeakMeasurements.begin(); Measurement != _Analysis->_PeakMeasurements.end(); ++Measurement)
            {
                if (_State->_HasCenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _GraphOptions, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _GraphOptions, &(*Measurement)));

                IsFirstBar = false;
            }
        }
    }
    else
    {
        if (_GraphOptions->_FlipHorizontally)
        {
            for (auto Measurement = _Analysis->_PeakMeasurements.rbegin(); Measurement != _Analysis->_PeakMeasurements.rend(); ++Measurement)
            {
                if (_State->_HasCenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _GraphOptions, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _GraphOptions, &(*Measurement)));

                IsFirstBar = false;
            }
        }
        else
        {
            for (auto Measurement = _Analysis->_PeakMeasurements.begin(); Measurement != _Analysis->_PeakMeasurements.end(); ++Measurement)
            {
                if (_State->_HasCenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _GraphOptions, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _GraphOptions, &(*Measurement)));

                IsFirstBar = false;
            }
        }
    }

    if (_GraphOptions->_YAxisRight)
    {
        _Parts.push_back(new scale_t
        (
            _State, _GraphOptions,
            _State->_IsHorizontalPeakMeter ? DWRITE_TEXT_ALIGNMENT_CENTER: DWRITE_TEXT_ALIGNMENT_LEADING,
            _State->_IsHorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER
        ));
    }
}

/// <summary>
/// Deletes the parts of this instance.
/// </summary>
void peak_meter_t::DeleteParts() noexcept
{
    for (auto Part : _Parts)
        delete Part;

    _Parts.clear();
}

/// <summary>
/// Measures the parts after the Direct 2D resources have been assigned.
/// </summary>
void peak_meter_t::MeasureParts(ID2D1DeviceContext * deviceContext) noexcept
{
    uint32_t BarCount = 0;

    FLOAT TotalScaleWidth  = 0.f;
    FLOAT TotalScaleHeight = 0.f;

    // Calculate how much space the scales occupy.
    for (auto Part : _Parts)
    {
        Part->Bind
        (
            deviceContext,
            &_BackgroundStyle,
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
            _DebugBrush,
            _OpacityMask
        );

        auto * Scale = dynamic_cast<scale_t *>(Part);

        if (Scale != nullptr)
        {
            if (_State->_IsHorizontalPeakMeter)
                TotalScaleHeight += _ScaleTextStyle._Height + (Scale->IsCenter() ? 0.f : _TickSize);
            else
                TotalScaleWidth  += _ScaleTextStyle._Width  + (Scale->IsCenter() ? 0.f : _TickSize);
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
            auto * Scale = dynamic_cast<scale_t *>(Part);

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

            Part->SetRect(Rect);

            y += Rect.bottom - Rect.top;
        }
    }
    else
    {
        FLOAT x = _Rect.left + Offset;

        for (auto & Part : _Parts)
        {
            auto * Scale = dynamic_cast<scale_t *>(Part);

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

            Part->SetRect(Rect);

            x += Rect.right - Rect.left;
        }
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT peak_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    if (_BackgroundStyle._Brush == nullptr)
    {
        _BackgroundStyle = *_State->_StyleManager.GetStyle(VisualElement::BarBackground);

        _BackgroundStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _BackgroundStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_PeakStyle._Brush == nullptr)
    {
        _PeakStyle = *_State->_StyleManager.GetStyle(VisualElement::BarPeakLevel);

        _PeakStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _PeakStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_Peak0dBStyle._Brush == nullptr)
    {
        _Peak0dBStyle = *_State->_StyleManager.GetStyle(VisualElement::Bar0dBPeakLevel);

        _Peak0dBStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _Peak0dBStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_PeakTextStyle._Brush == nullptr)
    {
        _PeakTextStyle = *_State->_StyleManager.GetStyle(VisualElement::BarPeakLevelText);

        _PeakTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _PeakTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"+199.9", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_MaxPeakStyle._Brush == nullptr)
    {
        _MaxPeakStyle = *_State->_StyleManager.GetStyle(VisualElement::BarMaxPeakLevel);

        _MaxPeakStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _MaxPeakStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_RMSStyle._Brush == nullptr)
    {
        _RMSStyle = *_State->_StyleManager.GetStyle(VisualElement::BarRMSLevel);

        _RMSStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _RMSStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_RMS0dBStyle._Brush == nullptr)
    {
        _RMS0dBStyle = *_State->_StyleManager.GetStyle(VisualElement::Bar0dBRMSLevel);

        _RMS0dBStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _RMS0dBStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_RMSTextStyle._Brush == nullptr)
    {
        _RMSTextStyle = *_State->_StyleManager.GetStyle(VisualElement::BarRMSLevelText);

        _RMSTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _RMSTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"+199.9", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_NameStyle._Brush == nullptr)
    {
        _NameStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisText);

        _NameStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _NameStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"LFE", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_ScaleTextStyle._Brush == nullptr)
    {
        _ScaleTextStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisText);

        _ScaleTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _ScaleTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"+999", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_ScaleLineStyle._Brush == nullptr)
    {
        _ScaleLineStyle = *_State->_StyleManager.GetStyle(VisualElement::HorizontalGridLine);

        _ScaleLineStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _ScaleLineStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"", 1.f);

        if (!SUCCEEDED(hr))
            return hr;
    }

    if (_OpacityMask == nullptr)
    {
        hr = CreateOpacityMask(deviceContext);

        if (!SUCCEEDED(hr))
            return hr;
    }

#ifdef _DEBUG
    if (_DebugBrush == nullptr)
    {
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);

        if (!SUCCEEDED(hr))
            return hr;
    }
#endif

    if (_RenderedChannels != _Analysis->_PeakMeasuredChannels)
    {
        DeleteParts();

        CreateParts();

        MeasureParts(deviceContext);

        _RenderedChannels = _Analysis->_PeakMeasuredChannels;
    }

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void peak_meter_t::DeleteDeviceSpecificResources() noexcept
{
    for (const auto & Part : _Parts)
        Part->Unbind();

    DeleteParts();

    _BackgroundStyle.DeleteDeviceSpecificResources();

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

    _OpacityMask.Release();

#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}

/// <summary>
/// Creates an opacity mask to render the LEDs.
/// </summary>
HRESULT peak_meter_t::CreateOpacityMask(ID2D1DeviceContext * deviceContext) noexcept
{
    CComPtr<ID2D1BitmapRenderTarget> rt;

    HRESULT hr = deviceContext->CreateCompatibleRenderTarget(D2D1::SizeF(_Size.width, _Size.height), &rt);

    if (SUCCEEDED(hr))
    {
        rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &Brush); // Black parts will be masked out.

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->Clear(); // Transparent

            const FLOAT LEDSize = _State->_LEDLight + _State->_LEDGap;

            if (LEDSize > 0.f)
            {
                if (_State->_IsHorizontalPeakMeter)
                {
                    for (FLOAT x = 0.f; x < _Size.width; x += LEDSize)
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDLight, _Size.height), Brush);
                }
                else
                {
                    for (FLOAT y = 0.f; y < _Size.height; y += LEDSize)
                        rt->FillRectangle(D2D1::RectF(0.f, y, _Size.width, y + _State->_LEDLight), Brush);
                }
            }

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}
