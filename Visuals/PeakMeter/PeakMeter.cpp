
/** $VER: PeakMeter.cpp (2025.11.09) P. Stuer - Represents a peak meter. **/

#include "pch.h"

#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

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
peak_meter_t::~peak_meter_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void peak_meter_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _Settings = settings;
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

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Required by FillOpacityMask().

    if (!SUCCEEDED(hr))
        return;

    for (auto Part : _Parts)
        Part->Render();
}

/// <summary>
/// Creates the parts of this instance (e.g. after resizing or a change in channel configuration)
/// </summary>
void peak_meter_t::CreateParts() noexcept
{
    if (_Settings->_YAxisLeft)
        _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_TRAILING, _State->_HorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_FAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

    bool IsFirstBar = true;

    if (_State->_HorizontalPeakMeter)
    {
        if (_Settings->_FlipVertically)
        {
            for (auto Measurement = _Analysis->_Measurements.rbegin(); Measurement != _Analysis->_Measurements.rend(); ++Measurement)
            {
                if (_State->_CenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _Settings, &(*Measurement)));

                IsFirstBar = false;
            }
        }
        else
        {
            for (auto Measurement = _Analysis->_Measurements.begin(); Measurement != _Analysis->_Measurements.end(); ++Measurement)
            {
                if (_State->_CenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _Settings, &(*Measurement)));

                IsFirstBar = false;
            }
        }
    }
    else
    {
        if (_Settings->_FlipHorizontally)
        {
            for (auto Measurement = _Analysis->_Measurements.rbegin(); Measurement != _Analysis->_Measurements.rend(); ++Measurement)
            {
                if (_State->_CenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _Settings, &(*Measurement)));

                IsFirstBar = false;
            }
        }
        else
        {
            for (auto Measurement = _Analysis->_Measurements.begin(); Measurement != _Analysis->_Measurements.end(); ++Measurement)
            {
                if (_State->_CenterScale && !IsFirstBar)
                    _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER));

                _Parts.push_back(new bar_t(_State, _Settings, &(*Measurement)));

                IsFirstBar = false;
            }
        }
    }

    if (_Settings->_YAxisRight)
        _Parts.push_back(new scale_t(_State, _Settings, DWRITE_TEXT_ALIGNMENT_LEADING, _State->_HorizontalPeakMeter ? DWRITE_PARAGRAPH_ALIGNMENT_NEAR : DWRITE_PARAGRAPH_ALIGNMENT_CENTER));
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
            _BackgroundStyle,
            _PeakStyle,
            _Peak0dBStyle,
            _MaxPeakStyle,
            _PeakTextStyle,
            _RMSStyle,
            _RMS0dBStyle,
            _RMSTextStyle,
            _NameStyle,
            _ScaleTextStyle,
            _ScaleLineStyle,
            _DebugBrush,
            _OpacityMask
        );

        auto * Scale = dynamic_cast<scale_t *>(Part);

        if (Scale != nullptr)
        {
            if (_State->_HorizontalPeakMeter)
                TotalScaleHeight += _ScaleTextStyle->_Height + (Scale->IsCenter() ? 0.f : _TickSize);
            else
                TotalScaleWidth  += _ScaleTextStyle->_Width  + (Scale->IsCenter() ? 0.f : _TickSize);
        }
        else
            ++BarCount;
    }

    const FLOAT TotalBarGap = _State->_CenterScale ? 0.f : _State->_BarGap * (FLOAT) (BarCount - 1);

    FLOAT Offset = 0.f;
    FLOAT BarWidth = 0.f;
    FLOAT BarHeight = 0.f;

    // Calculate the width / height of a bar and the offset on the graph.
    {
        if (_State->_HorizontalPeakMeter)
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

    if (_State->_HorizontalPeakMeter)
    {
        D2D1_RECT_F Rect = { 0.f, 0.f, _Size.width, 0.f };
        FLOAT y = Offset;

        for (auto & Part : _Parts)
        {
            auto * Scale = dynamic_cast<scale_t *>(Part);

            if (Scale != nullptr) // Scale
            {
                Rect.top    = y;
                Rect.bottom = y + _ScaleTextStyle->_Height + (Scale->IsCenter() ? 0.f : _TickSize);

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
        D2D1_RECT_F Rect = { 0.f, 0.f, 0.f, _Size.height };
        FLOAT x = Offset;

        for (auto & Part : _Parts)
        {
            auto * Scale = dynamic_cast<scale_t *>(Part);

            // A scale determines its own width.
            if (Scale != nullptr)
            {
                Rect.left  = x;
                Rect.right = x + _ScaleTextStyle->_Width + (Scale->IsCenter() ? 0.f : _TickSize);

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

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarBackground, deviceContext, _Size, L"", 1.f, &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarPeakLevel, deviceContext, _Size, L"", 1.f, &_PeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Bar0dBPeakLevel, deviceContext, _Size, L"", 1.f, &_Peak0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarPeakLevelText, deviceContext, _Size, L"+199.9", 1.f, &_PeakTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarMaxPeakLevel, deviceContext, _Size, L"", 1.f, &_MaxPeakStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarRMSLevel, deviceContext, _Size, L"", 1.f, &_RMSStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::Bar0dBRMSLevel, deviceContext, _Size, L"", 1.f, &_RMS0dBStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::BarRMSLevelText, deviceContext, _Size, L"+199.9", 1.f, &_RMSTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, deviceContext, _Size, L"LFE", 1.f, &_NameStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"+999", 1.f, &_ScaleTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_ScaleLineStyle);

    if (SUCCEEDED(hr) && (_OpacityMask == nullptr))
        hr = CreateOpacityMask(deviceContext);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    if (SUCCEEDED(hr) && (_RenderedChannels != _Analysis->_MeasuredChannels))
    {
        DeleteParts();

        CreateParts();

        MeasureParts(deviceContext);

        _RenderedChannels = _Analysis->_MeasuredChannels;
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

    SafeRelease(&_BackgroundStyle);

    SafeRelease(&_PeakStyle);
    SafeRelease(&_Peak0dBStyle);
    SafeRelease(&_MaxPeakStyle);
    SafeRelease(&_PeakTextStyle);

    SafeRelease(&_RMSStyle);
    SafeRelease(&_RMS0dBStyle);
    SafeRelease(&_RMSTextStyle);

    SafeRelease(&_NameStyle);

    SafeRelease(&_ScaleTextStyle);
    SafeRelease(&_ScaleLineStyle);

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
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &Brush); // Black parts will be masked out.

        if (SUCCEEDED(hr))
        {
            rt->BeginDraw();

            rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

            rt->Clear();

            const FLOAT LEDSize = _State->_LEDSize + _State->_LEDGap;

            if (LEDSize > 0.f)
            {
                if (_State->_HorizontalPeakMeter)
                {
                    for (FLOAT x = 0.f; x < _Size.width; x += LEDSize)
                        rt->FillRectangle(D2D1::RectF(x, 0.f, x + _State->_LEDSize, _Size.height), Brush);
                }
                else
                {
                    for (FLOAT y = 0.f; y < _Size.height; y += LEDSize)
                        rt->FillRectangle(D2D1::RectF(0.f, y, _Size.width, y + _State->_LEDSize), Brush);
                }
            }

            hr = rt->EndDraw();
        }

        if (SUCCEEDED(hr))
            hr = rt->GetBitmap(&_OpacityMask);
    }

    return hr;
}
