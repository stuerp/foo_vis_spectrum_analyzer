
/** $VER: Graph.cpp (2025.10.11) P. Stuer - Implements a graph on which the visualizations are rendered. **/

#include "pch.h"
#include "Graph.h"

#include "StyleManager.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
graph_t::graph_t()
{
}

/// <summary>
/// Destroys this instance.
/// </summary>
graph_t::~graph_t()
{
}

/// <summary>
/// Initializes this instance.
/// </summary>
void graph_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t *) noexcept
{
    _State = state;
    _GraphSettings = settings;

    _Description = settings->_Description;

    _Analysis.Initialize(state, settings);

    _Visualization.reset();

    switch (_State->_VisualizationType)
    {
        case VisualizationType::Bars:
        case VisualizationType::Curve:
        case VisualizationType::RadialBars:
        case VisualizationType::RadialCurve:
            _Visualization = std::make_unique<spectrum_t>();
            break;

        case VisualizationType::Spectogram:
            _Visualization = std::make_unique<spectogram_t>();
            break;

        case VisualizationType::PeakMeter:
            _Visualization = std::make_unique<peak_meter_t>();
            break;

        case VisualizationType::LevelMeter:
            _Visualization = std::make_unique<level_meter_t>();
            break;

        case VisualizationType::Oscilloscope:
            if (!_State->_XYMode)
                _Visualization = std::make_unique<oscilloscope_t>();
            else
                _Visualization = std::make_unique<oscilloscope_xy_t>();
            break;
    }

    _Visualization->Initialize(state, settings, &_Analysis);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void graph_t::Move(const D2D1_RECT_F & rect) noexcept
{
    const D2D1_RECT_F cr =
    {
        .left   = rect.left   + _GraphSettings->_LPadding,
        .top    = rect.top    + _GraphSettings->_TPadding,
        .right  = rect.right  - _GraphSettings->_RPadding,
        .bottom = rect.bottom - _GraphSettings->_BPadding
    };

    SetBounds(cr);

    _Visualization->Move(cr);
}

/// <summary>
/// Process a chunk of audio data.
/// </summary>
void graph_t::Process(const audio_chunk & chunk) noexcept
{
    _Analysis.Process(chunk); // Delegate it to the analysis.
}

/// <summary>
/// Renders this instance to the specified render target.
/// </summary>
void graph_t::Render(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (SUCCEEDED(hr))
    {
        RenderBackground(deviceContext, artwork);
        RenderForeground(deviceContext);
    }
}

/// <summary>
/// Resets this instance.
/// </summary>
void graph_t::Reset() noexcept
{
    for (frequency_band_t & fb : _Analysis._FrequencyBands)
        fb.CurValue = 0.;

    for (gauge_value_t & gv : _Analysis._GaugeValues)
    {
        gv.Peak = gv.RMS = -std::numeric_limits<double>::infinity();
        gv.PeakRender = gv.RMSRender = 0.;
    }

    _Visualization->Reset();
}

/// <summary>
/// Initializes a structure with the tool area of this graph.
/// </summary>
void graph_t::InitToolInfo(HWND hWnd, TTTOOLINFOW & ti) const noexcept
{
    ti = CToolInfo(TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE, hWnd, (UINT_PTR) hWnd, nullptr, nullptr);

    ::SetRect(&ti.rect, (int) _Bounds.left, (int) _Bounds.top, (int) _Bounds.right, (int) _Bounds.bottom);
}

/// <summary>
/// Gets the tooltip and frequency band index at the specified x or y position.
/// </summary>
bool graph_t::GetToolTipText(FLOAT x, FLOAT y, std::wstring & toolTip, size_t & bandIndex) const noexcept
{
    if ((_State->_VisualizationType == VisualizationType::Bars) || (_State->_VisualizationType == VisualizationType::Curve))
    {
        const rect_t & Bounds = (const rect_t &) _Visualization->GetClientBounds();

        FLOAT t = Bounds.Width() / (FLOAT) _Analysis._FrequencyBands.size();

        // Allow non-integer bar widths?
        if (_GraphSettings->_HorizontalAlignment != HorizontalAlignment::Fit)
            t = ::floor(t);

        const FLOAT BarWidth = std::max(t, 2.f);
        const FLOAT SpectrumWidth = (_State->_VisualizationType == VisualizationType::Bars) ? BarWidth * (FLOAT) _Analysis._FrequencyBands.size() : Bounds.Width();
        const FLOAT HOffset = GetHOffset(_GraphSettings->_HorizontalAlignment, Bounds.Width() - SpectrumWidth);

        const FLOAT x1 = Bounds.x1 + HOffset;
        const FLOAT x2 = x1 + SpectrumWidth;

        if (!msc::InRange(x, x1, x2))
            return false;

        if (_GraphSettings->_FlipHorizontally)
            x = (x2 + x1) - x;

        bandIndex = std::clamp((size_t) ::floor(msc::Map(x, x1, x2, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
    }
    else
    if (_State->_VisualizationType == VisualizationType::Spectogram)
    {
        const D2D1_RECT_F & Bounds = _Visualization->GetClientBounds();

        if (_State->_HorizontalSpectogram)
        {
            if (!msc::InRange(y, Bounds.top, Bounds.bottom))
                return false;

            if (!_GraphSettings->_FlipVertically)
                y = (Bounds.bottom + Bounds.top) - y;

            bandIndex = std::clamp((size_t) ::floor(msc::Map(y, Bounds.top, Bounds.bottom, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
        }
        else
        {
            if (!msc::InRange(x, Bounds.left, Bounds.right))
                return false;

            bandIndex = std::clamp((size_t) ::floor(msc::Map(x, Bounds.left, Bounds.right, 0., (double) _Analysis._FrequencyBands.size())), (size_t) 0, _Analysis._FrequencyBands.size() - (size_t) 1);
        }
    }
    else
        return false; // No tooltip available.

    toolTip = _Analysis._FrequencyBands[bandIndex].Label;

    return true;
}

/// <summary>
/// Renders the background.
/// </summary>
void graph_t::RenderBackground(ID2D1DeviceContext * deviceContext, artwork_t & artwork) noexcept
{
    if (_BackgroundStyle->IsEnabled())
        deviceContext->FillRectangle(_Bounds, _BackgroundStyle->_Brush);

    if (!_State->_ShowArtworkOnBackground)
        return;

    if ((_State->_VisualizationType == VisualizationType::PeakMeter) || (_State->_VisualizationType == VisualizationType::LevelMeter))
        return;

    if (artwork.Bitmap() == nullptr)
        return;

    artwork.Render(deviceContext, _State->_FitWindow ? _Visualization->GetBounds() : _Visualization->GetClientBounds(), _State);
}

/// <summary>
/// Renders the foreground.
/// </summary>
void graph_t::RenderForeground(ID2D1DeviceContext * deviceContext) noexcept
{
    _Visualization->Render(deviceContext);

    if ((_State->_VisualizationType == VisualizationType::PeakMeter) || (_State->_VisualizationType == VisualizationType::LevelMeter))
        return;

    RenderDescription(deviceContext);
}

/// <summary>
/// Renders the description.
/// </summary>
void graph_t::RenderDescription(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_Description.empty())
        return;

    CComPtr<IDWriteTextLayout> TextLayout;

    HRESULT hr = _DirectWrite.Factory->CreateTextLayout(_Description.c_str(), (UINT32) _Description.length(), _DescriptionTextStyle->_TextFormat, _Size.width, _Size.height, &TextLayout);

    DWRITE_TEXT_METRICS TextMetrics = { };

    if (SUCCEEDED(hr))
        hr = TextLayout->GetMetrics(&TextMetrics);

    if (SUCCEEDED(hr))
    {
        const FLOAT Inset = 2.f;

        D2D1_RECT_F Rect =
        {
            .left = _Visualization->GetClientBounds().left + 10.f,
            .top  = _Visualization->GetClientBounds().top  + 10.f,
        };

        Rect.right  = Rect.left + TextMetrics.width  + (Inset * 2.f);
        Rect.bottom = Rect.top  + TextMetrics.height + (Inset * 2.f);

        if (_DescriptionBackgroundStyle->IsEnabled())
            deviceContext->FillRoundedRectangle(D2D1::RoundedRect(Rect, Inset, Inset), _DescriptionBackgroundStyle->_Brush);

        if (_DescriptionTextStyle->IsEnabled())
            deviceContext->DrawText(_Description.c_str(), (UINT) _Description.length(), _DescriptionTextStyle->_TextFormat, Rect, _DescriptionTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT graph_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphBackground, deviceContext, _Size, L"", 1.f, &_BackgroundStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionText, deviceContext, _Size, L"", 1.f, &_DescriptionTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::GraphDescriptionBackground, deviceContext, _Size, L"", 1.f, &_DescriptionBackgroundStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif
    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void graph_t::DeleteDeviceSpecificResources() noexcept
{
    _Visualization->DeleteDeviceSpecificResources();

    SafeRelease(&_DescriptionBackgroundStyle);
    SafeRelease(&_DescriptionTextStyle);
    SafeRelease(&_BackgroundStyle);

#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}
